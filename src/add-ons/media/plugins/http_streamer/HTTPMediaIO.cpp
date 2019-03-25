/*
 * Copyright 2016, Dario Casalinuovo
 * Distributed under the terms of the MIT License.
 */


#include "HTTPMediaIO.h"

#include <Handler.h>
#include <HttpRequest.h>
#include <UrlProtocolRoster.h>

#include "MediaDebug.h"

using namespace BCodecKit;


// 10 seconds timeout
#define HTTP_TIMEOUT 10000000


class FileListener : public BURLProtocolListener {
public:
		FileListener(HTTPMediaIO* owner)
			:
			BURLProtocolListener(),
			fRequest(NULL),
			fAdapterIO(owner),
			fInitSem(-1),
			fRunning(false)
		{
			fInputAdapter = fAdapterIO->BuildInputAdapter();
			fInitSem = create_sem(0, "http_streamer init sem");
		}

		virtual ~FileListener()
		{
			_ReleaseInit();
		}

		bool ConnectionSuccessful() const
		{
			return fRequest != NULL;
		}

		void ConnectionOpened(BURLRequest* request)
		{
			fRequest = request;
			fRunning = true;
		}

		void HeadersReceived(BURLRequest* request, const BURLResult& result)
		{
			fAdapterIO->UpdateSize();
		}

		void DataReceived(BURLRequest* request, const char* data,
			off_t position, ssize_t size)
		{
			if (request != fRequest) {
				delete request;
				return;
			}

			BHTTPRequest* httpReq = dynamic_cast<BHTTPRequest*>(request);
			if (httpReq != NULL) {
				const BHTTPResult& httpRes
					= (const BHTTPResult&)httpReq->Result();
				int32 status = httpRes.StatusCode();
				if (BHTTPRequest::IsClientErrorStatusCode(status)
						|| BHTTPRequest::IsServerErrorStatusCode(status)) {
					fRunning = false;
				} else if (BHTTPRequest::IsRedirectionStatusCode(status))
					return;
			}

			_ReleaseInit();

			fInputAdapter->Write(data, size);
		}

		void RequestCompleted(BURLRequest* request, bool success)
		{
			_ReleaseInit();

			if (request != fRequest)
				return;

			fRequest = NULL;
		}

		status_t LockOnInit(bigtime_t timeout)
		{
			return acquire_sem_etc(fInitSem, 1, B_RELATIVE_TIMEOUT, timeout);
		}

		bool IsRunning()
		{
			return fRunning;
		}

private:
		void _ReleaseInit()
		{
			if (fInitSem != -1) {
				release_sem(fInitSem);
				delete_sem(fInitSem);
				fInitSem = -1;
			}
		}

		BURLRequest*	fRequest;
		HTTPMediaIO*	fAdapterIO;
		BInputAdapter*	fInputAdapter;
		sem_id			fInitSem;
		bool			fRunning;
};


HTTPMediaIO::HTTPMediaIO(BURL url)
	:
	BAdapterIO(B_MEDIA_STREAMING | B_MEDIA_SEEKABLE, HTTP_TIMEOUT),
	fReq(NULL),
	fListener(NULL),
	fReqThread(-1),
	fUrl(url),
	fIsMutable(false)
{
	CALLED();
}


HTTPMediaIO::~HTTPMediaIO()
{
	CALLED();

	if (fReq != NULL) {
		fReq->Stop();
		status_t status;
		wait_for_thread(fReqThread, &status);

		delete fReq;
	}
}


void
HTTPMediaIO::GetFlags(int32* flags) const
{
	*flags = B_MEDIA_STREAMING | B_MEDIA_SEEK_BACKWARD;
	if (fIsMutable)
		*flags = *flags | B_MEDIA_MUTABLE_SIZE;
}


ssize_t
HTTPMediaIO::WriteAt(off_t position, const void* buffer, size_t size)
{
	return B_NOT_SUPPORTED;
}


status_t
HTTPMediaIO::SetSize(off_t size)
{
	return B_NOT_SUPPORTED;
}


status_t
HTTPMediaIO::Open()
{
	CALLED();

	fListener = new FileListener(this);

	fReq = BURLProtocolRoster::MakeRequest(fUrl, fListener);

	if (fReq == NULL)
		return B_ERROR;

	fReqThread = fReq->Run();
	if (fReqThread < 0)
		return B_ERROR;

	status_t ret = fListener->LockOnInit(HTTP_TIMEOUT);
	if (ret != B_OK)
		return ret;

	return BAdapterIO::Open();
}


bool
HTTPMediaIO::IsRunning() const
{
	if (fListener != NULL)
		return fListener->IsRunning();
	else if (fReq != NULL)
		return fReq->IsRunning();

	return false;
}


status_t
HTTPMediaIO::SeekRequested(off_t position)
{
	return BAdapterIO::SeekRequested(position);
}


void
HTTPMediaIO::UpdateSize()
{
	// At this point we decide if our size is fixed or mutable,
	// this will change the behavior of our parent.
	off_t size = fReq->Result().Length();
	if (size > 0)
		BAdapterIO::SetSize(size);
	else
		fIsMutable = true;
}
