/*
 * Copyright 2017-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "AbstractServerProcess.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <AutoDeleter.h>
#include <ExclusiveBorrow.h>
#include <File.h>
#include <FileIO.h>
#include <HttpFields.h>
#include <HttpRequest.h>
#include <HttpResult.h>
#include <HttpTime.h>
#include <NetServicesDefs.h>
#include <ZlibCompressionAlgorithm.h>

#include "DataIOUtils.h"
#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "ServerHelper.h"
#include "ServerSettings.h"
#include "StandardMetaDataJsonEventListener.h"
#include "StorageUtils.h"


using namespace BPrivate::Network;


#define MAX_REDIRECTS 3
#define MAX_FAILURES 2


// 30 seconds
#define TIMEOUT_MICROSECONDS 3e+7


AbstractServerProcess::AbstractServerProcess(uint32 options)
	:
	AbstractProcess(),
	fOptions(options)
{
}


AbstractServerProcess::~AbstractServerProcess()
{
}


bool
AbstractServerProcess::HasOption(uint32 flag)
{
	return (fOptions & flag) == flag;
}


bool
AbstractServerProcess::ShouldAttemptNetworkDownload(bool hasDataAlready)
{
	return
		!HasOption(SERVER_PROCESS_NO_NETWORKING)
		&& !(HasOption(SERVER_PROCESS_PREFER_CACHE) && hasDataAlready);
}


status_t
AbstractServerProcess::StopInternal()
{
	if (fCurrentRequestIdentifier.has_value())
		ServerHelper::GetHttpSession().Cancel(*fCurrentRequestIdentifier);

	return AbstractProcess::StopInternal();
}


status_t
AbstractServerProcess::IfModifiedSinceHeaderValue(BString& headerValue) const
{
	BPath metaDataPath;
	BString jsonPath;

	status_t result = GetStandardMetaDataPath(metaDataPath);

	if (result != B_OK)
		return result;

	GetStandardMetaDataJsonPath(jsonPath);

	return IfModifiedSinceHeaderValue(headerValue, metaDataPath, jsonPath);
}


status_t
AbstractServerProcess::IfModifiedSinceHeaderValue(BString& headerValue,
	const BPath& metaDataPath, const BString& jsonPath) const
{
	headerValue.SetTo("");
	struct stat s;

	if (-1 == stat(metaDataPath.Path(), &s)) {
		if (ENOENT != errno)
			 return B_ERROR;

		return B_ENTRY_NOT_FOUND;
	}

	if (s.st_size == 0)
		return B_BAD_VALUE;

	StandardMetaData metaData;
	status_t result = PopulateMetaData(metaData, metaDataPath, jsonPath);

	if (result == B_OK)
		SetIfModifiedSinceHeaderValueFromMetaData(headerValue, metaData);
	else {
		HDERROR("unable to parse the meta-data date and time from [%s]"
			" - cannot set the 'If-Modified-Since' header",
			metaDataPath.Path());
	}

	return result;
}


/*static*/ void
AbstractServerProcess::SetIfModifiedSinceHeaderValueFromMetaData(
	BString& headerValue,
	const StandardMetaData& metaData)
{
	// An example of this output would be; 'Fri, 24 Oct 2014 19:32:27 +0000'
	BDateTime modifiedDateTime = metaData
		.GetDataModifiedTimestampAsDateTime();

	headerValue = std::move(format_http_time(modifiedDateTime));
}


status_t
AbstractServerProcess::PopulateMetaData(
	StandardMetaData& metaData, const BPath& path,
	const BString& jsonPath) const
{
	StandardMetaDataJsonEventListener listener(jsonPath, metaData);
	status_t result = ParseJsonFromFileWithListener(&listener, path);

	if (result != B_OK)
		return result;

	result = listener.ErrorStatus();

	if (result != B_OK)
		return result;

	if (!metaData.IsPopulated()) {
		HDERROR("the meta data was read from [%s], but no values "
			"were extracted", path.Path());
		return B_BAD_DATA;
	}

	return B_OK;
}


/* static */ bool
AbstractServerProcess::_LooksLikeGzip(const char *pathStr)
{
	int l = strlen(pathStr);
	return l > 4 && 0 == strncmp(&pathStr[l - 3], ".gz", 3);
}


/*!	Note that a B_OK return code from this method may not indicate that the
	listening process went well.  One has to see if there was an error in
	the listener.
*/

status_t
AbstractServerProcess::ParseJsonFromFileWithListener(
	BJsonEventListener *listener,
	const BPath& path) const
{
	const char* pathStr = path.Path();
	FILE* file = fopen(pathStr, "rb");

	if (file == NULL) {
		HDERROR("[%s] unable to find the meta data file at [%s]", Name(),
			path.Path());
		return B_ENTRY_NOT_FOUND;
	}

	BFileIO rawInput(file, true); // takes ownership

		// if the file extension ends with '.gz' then the data will be
		// compressed and the algorithm needs to decompress the data as
		// it is parsed.

	if (_LooksLikeGzip(pathStr)) {
		BDataIO* gzDecompressedInput = NULL;
		BZlibDecompressionParameters* zlibDecompressionParameters
			= new BZlibDecompressionParameters();

		status_t result = BZlibCompressionAlgorithm()
			.CreateDecompressingInputStream(&rawInput,
				zlibDecompressionParameters, gzDecompressedInput);

		if (B_OK != result)
			return result;

		ObjectDeleter<BDataIO> gzDecompressedInputDeleter(gzDecompressedInput);
		BPrivate::BJson::Parse(gzDecompressedInput, listener);
	} else {
		BPrivate::BJson::Parse(&rawInput, listener);
	}

	return B_OK;
}


/*! In order to reduce the chance of failure half way through downloading a
    large file, this method will download the file to a temporary file and
    then it can rename the file to the final target file.
*/

status_t
AbstractServerProcess::DownloadToLocalFileAtomically(
	const BPath& targetFilePath,
	const BUrl& url)
{
	BPath temporaryFilePath(tmpnam(NULL), NULL, true);
	status_t result = DownloadToLocalFile(
		temporaryFilePath, url, 0, 0);

	// if the data is coming in as .gz, but is not stored as .gz then
	// the data should be decompressed in the temporary file location
	// before being shifted into place.

	if (result == B_OK
			&& _LooksLikeGzip(url.Path())
			&& !_LooksLikeGzip(targetFilePath.Path()))
		result = _DeGzipInSitu(temporaryFilePath);

		// not copying if the data has not changed because the data will be
		// zero length.  This is if the result is APP_ERR_NOT_MODIFIED.
	if (result == B_OK) {
			// if the file is zero length then assume that something has
			// gone wrong.
		off_t size;
		bool hasFile;

		result = StorageUtils::ExistsObject(temporaryFilePath, &hasFile, NULL,
			&size);

		if (result == B_OK && hasFile && size > 0) {
			if (rename(temporaryFilePath.Path(), targetFilePath.Path()) != 0) {
				HDINFO("[%s] did rename [%s] --> [%s]",
					Name(), temporaryFilePath.Path(), targetFilePath.Path());
				result = B_IO_ERROR;
			}
		}
	}

	return result;
}


/*static*/ status_t
AbstractServerProcess::_DeGzipInSitu(const BPath& path)
{
	const char* tmpPath = tmpnam(NULL);
	status_t result = B_OK;

	{
		BFile file(path.Path(), O_RDONLY);
		BFile tmpFile(tmpPath, O_WRONLY | O_CREAT);

		BDataIO* gzDecompressedInput = NULL;
		BZlibDecompressionParameters* zlibDecompressionParameters
			= new BZlibDecompressionParameters();

		result = BZlibCompressionAlgorithm()
			.CreateDecompressingInputStream(&file,
				zlibDecompressionParameters, gzDecompressedInput);

		if (result == B_OK) {
			ObjectDeleter<BDataIO> gzDecompressedInputDeleter(
				gzDecompressedInput);
			result = DataIOUtils::CopyAll(&tmpFile, gzDecompressedInput);
		}
	}

	if (result == B_OK) {
		if (rename(tmpPath, path.Path()) != 0) {
			HDERROR("unable to move the uncompressed data into place");
			result = B_ERROR;
		}
	}
	else {
		HDERROR("it was not possible to decompress the data");
	}

	return result;
}


status_t
AbstractServerProcess::DownloadToLocalFile(const BPath& targetFilePath,
	const BUrl& url, uint32 redirects, uint32 failures)
{
	if (WasStopped())
		return B_CANCELED;

	if (redirects > MAX_REDIRECTS) {
		HDINFO("[%s] exceeded %d redirects --> failure", Name(),
			MAX_REDIRECTS);
		return B_IO_ERROR;
	}

	if (failures > MAX_FAILURES) {
		HDINFO("[%s] exceeded %d failures", Name(), MAX_FAILURES);
		return B_IO_ERROR;
	}

	HDINFO("[%s] will stream '%s' to [%s]", Name(), url.UrlString().String(),
		targetFilePath.Path());

	auto targetFile = make_exclusive_borrow<BFile>(targetFilePath.Path(), O_WRONLY | O_CREAT);
	status_t err = targetFile->InitCheck();
	if (err != B_OK)
		return err;

	BHttpFields fields;
	ServerSettings::AugmentHeaders(fields);

	BString ifModifiedSinceHeader;
	status_t ifModifiedSinceHeaderStatus = IfModifiedSinceHeaderValue(
		ifModifiedSinceHeader);

	if (ifModifiedSinceHeaderStatus == B_OK &&
		ifModifiedSinceHeader.Length() > 0) {
		fields.AddField("If-Modified-Since", ifModifiedSinceHeader.String());
	}

	auto request = BHttpRequest(url);
	request.SetFields(fields);
	request.SetMaxRedirections(0);
	request.SetTimeout(TIMEOUT_MICROSECONDS);
	request.SetStopOnError(true);

	auto result =
		ServerHelper::GetHttpSession().Execute(std::move(request), BBorrow<BDataIO>(targetFile));
	fCurrentRequestIdentifier = result.Identity();
	try {
		auto status = result.Status();
		if (status.StatusClass() == BHttpStatusClass::Success) {
			result.Body();
			HDINFO("[%s] did complete streaming data", Name());
			fCurrentRequestIdentifier = std::nullopt;
			return B_OK;
		} else if (status.StatusCode() == BHttpStatusCode::NotModified) {
			HDINFO("[%s] remote data has not changed since [%s]", Name(),
				ifModifiedSinceHeader.String());
			fCurrentRequestIdentifier = std::nullopt;
			return HD_ERR_NOT_MODIFIED;
		} else if (status.StatusCode() == BHttpStatusCode::PreconditionFailed) {
			auto responseFields = result.Fields();
			ServerHelper::NotifyClientTooOld(responseFields);
			fCurrentRequestIdentifier = std::nullopt;
			return HD_CLIENT_TOO_OLD;
		} else if (status.StatusClass() == BHttpStatusClass::Redirection) {
			auto responseFields = result.Fields();
			auto locationField = responseFields.FindField("Location");
			if (locationField != responseFields.end()) {
				BString location(locationField->Value().data(), locationField->Value().size());
				BUrl redirectUrl(url, location);
				HDINFO("[%s] will redirect to; %s",
					Name(), redirectUrl.UrlString().String());
				fCurrentRequestIdentifier = std::nullopt;
				return DownloadToLocalFile(targetFilePath, redirectUrl,
					redirects + 1, 0);
			} else {
				HDERROR("[%s] unable to find 'Location' header for redirect", Name());
				fCurrentRequestIdentifier = std::nullopt;
				return B_IO_ERROR;
			}
		} else if (status.StatusClass() == BHttpStatusClass::ServerError) {
			HDERROR("error response from server [%" B_PRId16 "] --> retry...",
				status.code);
			fCurrentRequestIdentifier = std::nullopt;
			return DownloadToLocalFile(targetFilePath, url, redirects,
				failures + 1);
		} else {
			HDERROR("[%s] unexpected response from server [%" B_PRId16 "]",
				Name(), status.code);
			fCurrentRequestIdentifier = std::nullopt;
			return B_IO_ERROR;
		}
	} catch (const BNetworkRequestError& error) {
		fCurrentRequestIdentifier = std::nullopt;
		if (error.Type() == BNetworkRequestError::NetworkError) {
			HDERROR("network error while trying to execute request [%s] --> retry...",
				error.DebugMessage().String());
			return DownloadToLocalFile(targetFilePath, url, redirects,
				failures + 1);
		}
		HDERROR("[%s] unexpected error while executing request [%s]",
			Name(), error.DebugMessage().String());
		return B_IO_ERROR;
	}
}


status_t
AbstractServerProcess::DeleteLocalFile(const BPath& currentFilePath)
{
	if (0 == remove(currentFilePath.Path()))
		return B_OK;

	return B_IO_ERROR;
}


/*!	When a file is damaged or corrupted in some way, the file should be 'moved
    aside' so that it is not involved in the next update.  This method will
    create such an alternative 'damaged' file location and move this file to
    that location.
*/

status_t
AbstractServerProcess::MoveDamagedFileAside(const BPath& currentFilePath)
{
	BPath damagedFilePath;
	BString damagedLeaf;

	damagedLeaf.SetToFormat("%s__damaged", currentFilePath.Leaf());
	currentFilePath.GetParent(&damagedFilePath);
	damagedFilePath.Append(damagedLeaf.String());

	if (0 != rename(currentFilePath.Path(), damagedFilePath.Path())) {
		HDERROR("[%s] unable to move damaged file [%s] aside to [%s]",
			Name(), currentFilePath.Path(), damagedFilePath.Path());
		return B_IO_ERROR;
	}

	HDINFO("[%s] did move damaged file [%s] aside to [%s]",
		Name(), currentFilePath.Path(), damagedFilePath.Path());

	return B_OK;
}


bool
AbstractServerProcess::IsSuccess(status_t e) {
	return e == B_OK || e == HD_ERR_NOT_MODIFIED;
}
