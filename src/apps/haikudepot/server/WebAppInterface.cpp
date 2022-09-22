/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2022, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "WebAppInterface.h"

#include <AutoDeleter.h>
#include <Application.h>
#include <HttpFields.h>
#include <HttpRequest.h>
#include <HttpResult.h>
#include <Json.h>
#include <JsonTextWriter.h>
#include <JsonMessageWriter.h>
#include <Message.h>
#include <Messenger.h>
#include <NetServicesDefs.h>
#include <Url.h>

#include "DataIOUtils.h"
#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "ServerSettings.h"
#include "ServerHelper.h"


using namespace BPrivate::Network;


#define BASEURL_DEFAULT "https://depot.haiku-os.org"
#define USERAGENT_FALLBACK_VERSION "0.0.0"
#define PROTOCOL_NAME "post-json"
#define LOG_PAYLOAD_LIMIT 8192


int
WebAppInterface::fRequestIndex = 0;


enum {
	NEEDS_AUTHORIZATION = 1 << 0,
};


WebAppInterface::WebAppInterface()
{
}


WebAppInterface::WebAppInterface(const WebAppInterface& other)
	:
	fCredentials(other.fCredentials)
{
}


WebAppInterface::~WebAppInterface()
{
}


WebAppInterface&
WebAppInterface::operator=(const WebAppInterface& other)
{
	if (this == &other)
		return *this;
	fCredentials = other.fCredentials;
	return *this;
}


void
WebAppInterface::SetAuthorization(const UserCredentials& value)
{
	fCredentials = value;
}


const BString&
WebAppInterface::Nickname() const
{
	return fCredentials.Nickname();
}


status_t
WebAppInterface::GetChangelog(const BString& packageName, BMessage& message)
{
	auto requestEnvelopeData = std::make_unique<BMallocIO>();
	BJsonTextWriter requestEnvelopeWriter(requestEnvelopeData.get());

	requestEnvelopeWriter.WriteObjectStart();
	requestEnvelopeWriter.WriteObjectName("pkgName");
	requestEnvelopeWriter.WriteString(packageName.String());
	requestEnvelopeWriter.WriteObjectEnd();

	return _SendJsonRequest("pkg/get-pkg-change-log",
		std::move(requestEnvelopeData), _LengthAndSeekToZero(requestEnvelopeData.get()),
		0, message);
}


status_t
WebAppInterface::RetreiveUserRatingsForPackageForDisplay(
	const BString& packageName,
	const BString& webAppRepositoryCode,
	const BString& webAppRepositorySourceCode,
	int resultOffset, int maxResults, BMessage& message)
{
	auto requestEnvelopeData = std::make_unique<BMallocIO>();
	BJsonTextWriter requestEnvelopeWriter(requestEnvelopeData.get());

	requestEnvelopeWriter.WriteObjectStart();
	requestEnvelopeWriter.WriteObjectName("pkgName");
	requestEnvelopeWriter.WriteString(packageName.String());
	requestEnvelopeWriter.WriteObjectName("offset");
	requestEnvelopeWriter.WriteInteger(resultOffset);
	requestEnvelopeWriter.WriteObjectName("limit");
	requestEnvelopeWriter.WriteInteger(maxResults);

	if (!webAppRepositorySourceCode.IsEmpty()) {
		requestEnvelopeWriter.WriteObjectName("repositorySourceCode");
		requestEnvelopeWriter.WriteString(webAppRepositorySourceCode);
	}

	if (!webAppRepositoryCode.IsEmpty()) {
		requestEnvelopeWriter.WriteObjectName("repositoryCode");
		requestEnvelopeWriter.WriteString(webAppRepositoryCode);
	}

	requestEnvelopeWriter.WriteObjectEnd();

	return _SendJsonRequest("user-rating/search-user-ratings",
		std::move(requestEnvelopeData), _LengthAndSeekToZero(requestEnvelopeData.get()),
		0, message);
}


status_t
WebAppInterface::RetreiveUserRatingForPackageAndVersionByUser(
	const BString& packageName, const BPackageVersion& version,
	const BString& architecture,
	const BString& webAppRepositoryCode,
	const BString& webAppRepositorySourceCode,
	const BString& userNickname, BMessage& message)
{
		// BHttpRequest later takes ownership of this.
	auto requestEnvelopeData = std::make_unique<BMallocIO>();
	BJsonTextWriter requestEnvelopeWriter(requestEnvelopeData.get());

	requestEnvelopeWriter.WriteObjectStart();

	requestEnvelopeWriter.WriteObjectName("userNickname");
	requestEnvelopeWriter.WriteString(userNickname.String());
	requestEnvelopeWriter.WriteObjectName("pkgName");
	requestEnvelopeWriter.WriteString(packageName.String());
	requestEnvelopeWriter.WriteObjectName("pkgVersionArchitectureCode");
	requestEnvelopeWriter.WriteString(architecture.String());
	requestEnvelopeWriter.WriteObjectName("repositoryCode");
	requestEnvelopeWriter.WriteString(webAppRepositoryCode.String());
	requestEnvelopeWriter.WriteObjectName("repositorySourceCode");
	requestEnvelopeWriter.WriteString(webAppRepositorySourceCode.String());

	if (version.Major().Length() > 0) {
		requestEnvelopeWriter.WriteObjectName("pkgVersionMajor");
		requestEnvelopeWriter.WriteString(version.Major().String());
	}

	if (version.Minor().Length() > 0) {
		requestEnvelopeWriter.WriteObjectName("pkgVersionMinor");
		requestEnvelopeWriter.WriteString(version.Minor().String());
	}

	if (version.Micro().Length() > 0) {
		requestEnvelopeWriter.WriteObjectName("pkgVersionMicro");
		requestEnvelopeWriter.WriteString(version.Micro().String());
	}

	if (version.PreRelease().Length() > 0) {
		requestEnvelopeWriter.WriteObjectName("pkgVersionPreRelease");
		requestEnvelopeWriter.WriteString(version.PreRelease().String());
	}

	if (version.Revision() != 0) {
		requestEnvelopeWriter.WriteObjectName("pkgVersionRevision");
		requestEnvelopeWriter.WriteInteger(version.Revision());
	}

	requestEnvelopeWriter.WriteObjectEnd();

	return _SendJsonRequest(
		"user-rating/get-user-rating-by-user-and-pkg-version",
		std::move(requestEnvelopeData),
		_LengthAndSeekToZero(requestEnvelopeData.get()), NEEDS_AUTHORIZATION,
		message);
}


/*!	This method will fill out the supplied UserDetail object with information
	about the user that is supplied in the credentials.  Importantly it will
	also authenticate the request with the details of the credentials and will
	not use the credentials that are configured in 'fCredentials'.
*/

status_t
WebAppInterface::RetrieveUserDetailForCredentials(
	const UserCredentials& credentials, BMessage& message)
{
	if (!credentials.IsValid()) {
		debugger("the credentials supplied are invalid so it is not possible "
			"to obtain the user detail");
	}

		// BHttpRequest later takes ownership of this.
	auto requestEnvelopeData = std::make_unique<BMallocIO>();
	BJsonTextWriter requestEnvelopeWriter(requestEnvelopeData.get());

	requestEnvelopeWriter.WriteObjectStart();
	requestEnvelopeWriter.WriteObjectName("nickname");
	requestEnvelopeWriter.WriteString(credentials.Nickname().String());
	requestEnvelopeWriter.WriteObjectEnd();

	status_t result = _SendJsonRequest("user/get-user", credentials,
		std::move(requestEnvelopeData), _LengthAndSeekToZero(requestEnvelopeData.get()),
		NEEDS_AUTHORIZATION, message);
		// note that the credentials used here are passed in as args.

	return result;
}


/*!	This method will return the credentials for the currently authenticated
	user.
*/

status_t
WebAppInterface::RetrieveCurrentUserDetail(BMessage& message)
{
	return RetrieveUserDetailForCredentials(fCredentials, message);
}


/*!	When the user requests user detail, the server sends back an envelope of
	response data.  This method will unpack the data into a model object.
	\return Not B_OK if something went wrong.
*/

/*static*/ status_t
WebAppInterface::UnpackUserDetail(BMessage& responseEnvelopeMessage,
	UserDetail& userDetail)
{
	BMessage resultMessage;
	status_t result = responseEnvelopeMessage.FindMessage(
		"result", &resultMessage);

	if (result != B_OK) {
		HDERROR("bad response envelope missing 'result' entry");
		return result;
	}

	BString nickname;
	result = resultMessage.FindString("nickname", &nickname);
	userDetail.SetNickname(nickname);

	BMessage agreementMessage;
	if (resultMessage.FindMessage("userUsageConditionsAgreement",
		&agreementMessage) == B_OK) {
		BString code;
		BDateTime agreedToTimestamp;
		BString userUsageConditionsCode;
		UserUsageConditionsAgreement agreement = userDetail.Agreement();
		bool isLatest;

		if (agreementMessage.FindString("userUsageConditionsCode",
			&userUsageConditionsCode) == B_OK) {
			agreement.SetCode(userUsageConditionsCode);
		}

		double timestampAgreedMillis;
		if (agreementMessage.FindDouble("timestampAgreed",
			&timestampAgreedMillis) == B_OK) {
			agreement.SetTimestampAgreed((uint64) timestampAgreedMillis);
		}

		if (agreementMessage.FindBool("isLatest", &isLatest)
			== B_OK) {
			agreement.SetIsLatest(isLatest);
		}

		userDetail.SetAgreement(agreement);
	}

	return result;
}


/*!	\brief Returns data relating to the user usage conditions

	\param code defines the version of the data to return or if empty then the
		latest is returned.

	This method will go to the server and get details relating to the user usage
	conditions.  It does this in two API calls; first gets the details (the
	minimum age) and in the second call, the text of the conditions is returned.
*/

status_t
WebAppInterface::RetrieveUserUsageConditions(const BString& code,
	UserUsageConditions& conditions)
{
	BMessage responseEnvelopeMessage;
	status_t result = _RetrieveUserUsageConditionsMeta(code,
		responseEnvelopeMessage);

	if (result != B_OK)
		return result;

	BMessage resultMessage;
	if (responseEnvelopeMessage.FindMessage("result", &resultMessage) != B_OK) {
		HDERROR("bad response envelope missing 'result' entry");
		return B_BAD_DATA;
	}

	BString metaDataCode;
	double metaDataMinimumAge;
	BString copyMarkdown;

	if ( (resultMessage.FindString("code", &metaDataCode) != B_OK)
			|| (resultMessage.FindDouble(
				"minimumAge", &metaDataMinimumAge) != B_OK) ) {
		HDERROR("unexpected response from server with missing user usage "
			"conditions data");
		return B_BAD_DATA;
	}

	auto copyMarkdownData = make_exclusive_borrow<BMallocIO>();
	result = _RetrieveUserUsageConditionsCopy(metaDataCode, BBorrow<BDataIO>(copyMarkdownData));

	if (result != B_OK)
		return result;

	conditions.SetCode(metaDataCode);
	conditions.SetMinimumAge(metaDataMinimumAge);
	conditions.SetCopyMarkdown(
		BString(static_cast<const char*>(copyMarkdownData->Buffer()),
			copyMarkdownData->BufferLength()));

	return B_OK;
}


status_t
WebAppInterface::AgreeUserUsageConditions(const BString& code,
	BMessage& responsePayload)
{
	auto requestEnvelopeData = std::make_unique<BMallocIO>();
	BJsonTextWriter requestEnvelopeWriter(requestEnvelopeData.get());

	requestEnvelopeWriter.WriteObjectStart();
	requestEnvelopeWriter.WriteObjectName("userUsageConditionsCode");
	requestEnvelopeWriter.WriteString(code.String());
	requestEnvelopeWriter.WriteObjectName("nickname");
	requestEnvelopeWriter.WriteString(fCredentials.Nickname());
	requestEnvelopeWriter.WriteObjectEnd();

	// now fetch this information into an object.

	return _SendJsonRequest("user/agree-user-usage-conditions",
		std::move(requestEnvelopeData), _LengthAndSeekToZero(requestEnvelopeData.get()),
		NEEDS_AUTHORIZATION, responsePayload);
}


status_t
WebAppInterface::_RetrieveUserUsageConditionsMeta(const BString& code,
	BMessage& message)
{
	auto requestEnvelopeData = std::make_unique<BMallocIO>();
	BJsonTextWriter requestEnvelopeWriter(requestEnvelopeData.get());

	requestEnvelopeWriter.WriteObjectStart();

	if (!code.IsEmpty()) {
		requestEnvelopeWriter.WriteObjectName("code");
		requestEnvelopeWriter.WriteString(code.String());
	}

	requestEnvelopeWriter.WriteObjectEnd();

	// now fetch this information into an object.

	return _SendJsonRequest("user/get-user-usage-conditions",
		std::move(requestEnvelopeData), _LengthAndSeekToZero(requestEnvelopeData.get()),
		0, message);
}


status_t
WebAppInterface::_RetrieveUserUsageConditionsCopy(const BString& code,
	BBorrow<BDataIO> stream)
{
	return _SendRawGetRequest(
		BString("/__user/usageconditions/") << code << "/document.md",
		std::move(stream));
}


status_t
WebAppInterface::CreateUserRating(const BString& packageName,
	const BPackageVersion& version,
	const BString& architecture,
	const BString& webAppRepositoryCode,
	const BString& webAppRepositorySourceCode,
	const BString& languageCode, const BString& comment,
	const BString& stability, int rating, BMessage& message)
{
	auto requestEnvelopeData = std::make_unique<BMallocIO>();
	BJsonTextWriter requestEnvelopeWriter(requestEnvelopeData.get());

	requestEnvelopeWriter.WriteObjectStart();
	requestEnvelopeWriter.WriteObjectName("pkgName");
	requestEnvelopeWriter.WriteString(packageName.String());
	requestEnvelopeWriter.WriteObjectName("pkgVersionArchitectureCode");
	requestEnvelopeWriter.WriteString(architecture.String());
	requestEnvelopeWriter.WriteObjectName("repositoryCode");
	requestEnvelopeWriter.WriteString(webAppRepositoryCode.String());
	requestEnvelopeWriter.WriteObjectName("repositorySourceCode");
	requestEnvelopeWriter.WriteString(webAppRepositorySourceCode.String());
	requestEnvelopeWriter.WriteObjectName("naturalLanguageCode");
	requestEnvelopeWriter.WriteString(languageCode.String());
	requestEnvelopeWriter.WriteObjectName("pkgVersionType");
	requestEnvelopeWriter.WriteString("SPECIFIC");
	requestEnvelopeWriter.WriteObjectName("userNickname");
	requestEnvelopeWriter.WriteString(fCredentials.Nickname());

	if (!version.Major().IsEmpty()) {
		requestEnvelopeWriter.WriteObjectName("pkgVersionMajor");
		requestEnvelopeWriter.WriteString(version.Major());
	}

	if (!version.Minor().IsEmpty()) {
		requestEnvelopeWriter.WriteObjectName("pkgVersionMinor");
		requestEnvelopeWriter.WriteString(version.Minor());
	}

	if (!version.Micro().IsEmpty()) {
		requestEnvelopeWriter.WriteObjectName("pkgVersionMicro");
		requestEnvelopeWriter.WriteString(version.Micro());
	}

	if (!version.PreRelease().IsEmpty()) {
		requestEnvelopeWriter.WriteObjectName("pkgVersionPreRelease");
		requestEnvelopeWriter.WriteString(version.PreRelease());
	}

	if (version.Revision() != 0) {
		requestEnvelopeWriter.WriteObjectName("pkgVersionRevision");
		requestEnvelopeWriter.WriteInteger(version.Revision());
	}

	if (rating > 0.0f) {
		requestEnvelopeWriter.WriteObjectName("rating");
    	requestEnvelopeWriter.WriteInteger(rating);
	}

	if (stability.Length() > 0) {
		requestEnvelopeWriter.WriteObjectName("userRatingStabilityCode");
		requestEnvelopeWriter.WriteString(stability);
	}

	if (comment.Length() > 0) {
		requestEnvelopeWriter.WriteObjectName("comment");
		requestEnvelopeWriter.WriteString(comment.String());
	}

	requestEnvelopeWriter.WriteObjectEnd();

	return _SendJsonRequest("user-rating/create-user-rating",
		std::move(requestEnvelopeData), _LengthAndSeekToZero(requestEnvelopeData.get()),
		NEEDS_AUTHORIZATION, message);
}


status_t
WebAppInterface::UpdateUserRating(const BString& ratingID,
	const BString& languageCode, const BString& comment,
	const BString& stability, int rating, bool active, BMessage& message)
{
	auto requestEnvelopeData = std::make_unique<BMallocIO>();
	BJsonTextWriter requestEnvelopeWriter(requestEnvelopeData.get());

	requestEnvelopeWriter.WriteObjectStart();

	requestEnvelopeWriter.WriteObjectName("code");
	requestEnvelopeWriter.WriteString(ratingID.String());
	requestEnvelopeWriter.WriteObjectName("naturalLanguageCode");
	requestEnvelopeWriter.WriteString(languageCode.String());
	requestEnvelopeWriter.WriteObjectName("active");
	requestEnvelopeWriter.WriteBoolean(active);

	requestEnvelopeWriter.WriteObjectName("filter");
	requestEnvelopeWriter.WriteArrayStart();
	requestEnvelopeWriter.WriteString("ACTIVE");
	requestEnvelopeWriter.WriteString("NATURALLANGUAGE");
	requestEnvelopeWriter.WriteString("USERRATINGSTABILITY");
	requestEnvelopeWriter.WriteString("COMMENT");
	requestEnvelopeWriter.WriteString("RATING");
	requestEnvelopeWriter.WriteArrayEnd();

	if (rating >= 0) {
		requestEnvelopeWriter.WriteObjectName("rating");
		requestEnvelopeWriter.WriteInteger(rating);
	}

	if (stability.Length() > 0) {
		requestEnvelopeWriter.WriteObjectName("userRatingStabilityCode");
		requestEnvelopeWriter.WriteString(stability);
	}

	if (comment.Length() > 0) {
		requestEnvelopeWriter.WriteObjectName("comment");
		requestEnvelopeWriter.WriteString(comment);
	}

	requestEnvelopeWriter.WriteObjectEnd();

	return _SendJsonRequest("user-rating/update-user-rating",
		std::move(requestEnvelopeData), _LengthAndSeekToZero(requestEnvelopeData.get()),
		NEEDS_AUTHORIZATION, message);
}


status_t
WebAppInterface::RetrieveScreenshot(const BString& code,
	int32 width, int32 height, BBorrow<BDataIO> stream)
{
	return _SendRawGetRequest(
		BString("/__pkgscreenshot/") << code << ".png" << "?tw="
			<< width << "&th=" << height, std::move(stream));
}


BHttpResult
WebAppInterface::RetrieveScreenshotAsync(const BString& code,
	int32 width, int32 height, BBorrow<BDataIO> stream, const BMessenger& target)
{
	BString urlPathComponents = BString("/__pkgscreenshot/") << code << ".png" << "?tw="
			<< width << "&th=" << height;
	BUrl url = ServerSettings::CreateFullUrl(urlPathComponents);
	return _SendRawGetRequestAsync(url, std::move(stream), target);
}


status_t
WebAppInterface::RequestCaptcha(BMessage& message)
{
	auto requestEnvelopeData = std::make_unique<BMallocIO>();
	BJsonTextWriter requestEnvelopeWriter(requestEnvelopeData.get());

	requestEnvelopeWriter.WriteObjectStart();
	requestEnvelopeWriter.WriteObjectEnd();

	return _SendJsonRequest("captcha/generate-captcha",
		std::move(requestEnvelopeData), _LengthAndSeekToZero(requestEnvelopeData.get()),
		0, message);
}


status_t
WebAppInterface::CreateUser(const BString& nickName,
	const BString& passwordClear, const BString& email,
	const BString& captchaToken, const BString& captchaResponse,
	const BString& languageCode, const BString& userUsageConditionsCode,
	BMessage& message)
{
		// BHttpRequest later takes ownership of this.
	auto requestEnvelopeData = std::make_unique<BMallocIO>();
	BJsonTextWriter requestEnvelopeWriter(requestEnvelopeData.get());

	requestEnvelopeWriter.WriteObjectStart();

	requestEnvelopeWriter.WriteObjectName("nickname");
	requestEnvelopeWriter.WriteString(nickName.String());
	requestEnvelopeWriter.WriteObjectName("passwordClear");
	requestEnvelopeWriter.WriteString(passwordClear.String());
	requestEnvelopeWriter.WriteObjectName("captchaToken");
	requestEnvelopeWriter.WriteString(captchaToken.String());
	requestEnvelopeWriter.WriteObjectName("captchaResponse");
	requestEnvelopeWriter.WriteString(captchaResponse.String());
	requestEnvelopeWriter.WriteObjectName("naturalLanguageCode");
	requestEnvelopeWriter.WriteString(languageCode.String());
	requestEnvelopeWriter.WriteObjectName("userUsageConditionsCode");
	requestEnvelopeWriter.WriteString(userUsageConditionsCode.String());

	if (!email.IsEmpty()) {
		requestEnvelopeWriter.WriteObjectName("email");
		requestEnvelopeWriter.WriteString(email.String());
	}

	requestEnvelopeWriter.WriteObjectEnd();

	return _SendJsonRequest("user/create-user", std::move(requestEnvelopeData),
		_LengthAndSeekToZero(requestEnvelopeData.get()), 0, message);
}


status_t
WebAppInterface::AuthenticateUser(const BString& nickName,
	const BString& passwordClear, BMessage& message)
{
	auto requestEnvelopeData = std::make_unique<BMallocIO>();
	BJsonTextWriter requestEnvelopeWriter(requestEnvelopeData.get());

	requestEnvelopeWriter.WriteObjectStart();

	requestEnvelopeWriter.WriteObjectName("nickname");
	requestEnvelopeWriter.WriteString(nickName.String());
	requestEnvelopeWriter.WriteObjectName("passwordClear");
	requestEnvelopeWriter.WriteString(passwordClear.String());

	requestEnvelopeWriter.WriteObjectEnd();

	return _SendJsonRequest("user/authenticate-user",
		std::move(requestEnvelopeData), _LengthAndSeekToZero(requestEnvelopeData.get()),
		0, message);
}


status_t
WebAppInterface::IncrementViewCounter(const PackageInfoRef package,
	const DepotInfoRef depot, BMessage& message)
{
	auto requestEnvelopeData = std::make_unique<BMallocIO>();
	BJsonTextWriter requestEnvelopeWriter(requestEnvelopeData.get());

	requestEnvelopeWriter.WriteObjectStart();

	requestEnvelopeWriter.WriteObjectName("architectureCode");
	requestEnvelopeWriter.WriteString(package->Architecture());
	requestEnvelopeWriter.WriteObjectName("repositoryCode");
	requestEnvelopeWriter.WriteString(depot->WebAppRepositoryCode());
	requestEnvelopeWriter.WriteObjectName("repositorySourceCode");
	requestEnvelopeWriter.WriteString(depot->WebAppRepositorySourceCode());
	requestEnvelopeWriter.WriteObjectName("name");
	requestEnvelopeWriter.WriteString(package->Name());

	const BPackageVersion version = package->Version();
	if (!version.Major().IsEmpty()) {
		requestEnvelopeWriter.WriteObjectName("major");
		requestEnvelopeWriter.WriteString(version.Major());
	}
	if (!version.Minor().IsEmpty()) {
		requestEnvelopeWriter.WriteObjectName("minor");
		requestEnvelopeWriter.WriteString(version.Minor());
	}
	if (!version.Micro().IsEmpty()) {
		requestEnvelopeWriter.WriteObjectName("micro");
		requestEnvelopeWriter.WriteString(version.Micro());
	}
	if (!version.PreRelease().IsEmpty()) {
		requestEnvelopeWriter.WriteObjectName("preRelease");
		requestEnvelopeWriter.WriteString(version.PreRelease());
	}
	if (version.Revision() != 0) {
		requestEnvelopeWriter.WriteObjectName("revision");
		requestEnvelopeWriter.WriteInteger(
			static_cast<int64>(version.Revision()));
	}

	requestEnvelopeWriter.WriteObjectEnd();

	return _SendJsonRequest("pkg/increment-view-counter",
		std::move(requestEnvelopeData), _LengthAndSeekToZero(requestEnvelopeData.get()),
		0, message);
}


/*!	JSON-RPC invocations return a response.  The response may be either
	a result or it may be an error depending on the response structure.
	If it is an error then there may be additional detail that is the
	error code and message.  This method will extract the error code
	from the response.  This method will return 0 if the payload does
	not look like an error.
*/

int32
WebAppInterface::ErrorCodeFromResponse(BMessage& responseEnvelopeMessage)
{
	BMessage error;
	double code;

	if (responseEnvelopeMessage.FindMessage("error", &error) == B_OK
		&& error.FindDouble("code", &code) == B_OK) {
		return (int32) code;
	}

	return 0;
}


// #pragma mark - private


status_t
WebAppInterface::_SendJsonRequest(const char* urlPathComponents,
	std::unique_ptr<BPositionIO> requestData, size_t requestDataSize, uint32 flags,
	BMessage& reply) const
{
	return _SendJsonRequest(urlPathComponents, fCredentials, std::move(requestData),
		requestDataSize, flags, reply);
}


status_t
WebAppInterface::_SendJsonRequest(const char* urlPathComponents,
	UserCredentials credentials, std::unique_ptr<BPositionIO> requestData,
	size_t requestDataSize, uint32 flags, BMessage& reply) const
{
	if (requestDataSize == 0) {
		HDINFO("%s; empty request payload", PROTOCOL_NAME);
		return B_ERROR;
	}

	if (!ServerHelper::IsNetworkAvailable()) {
		HDDEBUG("%s; dropping request to ...[%s] as network is not"
			" available", PROTOCOL_NAME, urlPathComponents);
		return HD_NETWORK_INACCESSIBLE;
	}

	if (ServerSettings::IsClientTooOld()) {
		HDDEBUG("%s; dropping request to ...[%s] as client is too old",
			PROTOCOL_NAME, urlPathComponents);
		return HD_CLIENT_TOO_OLD;
	}

	BUrl url = ServerSettings::CreateFullUrl(BString("/__api/v2/")
		<< urlPathComponents);
	HDDEBUG("%s; will make request to [%s]", PROTOCOL_NAME,
		url.UrlString().String());

	// If the request payload is logged then it must be copied to local memory
	// from the stream.  This then requires that the request data is then
	// delivered from memory.

	if (Logger::IsTraceEnabled()) {
		HDLOGPREFIX(LOG_LEVEL_TRACE)
		printf("%s request; ", PROTOCOL_NAME);
		_LogPayload(requestData.get(), requestDataSize);
		printf("\n");
	}

	BHttpFields fields;
	fields.AddField("Accept", "application/json");
	ServerSettings::AugmentHeaders(fields);

	BHttpRequest request(url);
	request.SetRequestBody(std::move(requestData), "application/json", requestDataSize);
	request.SetMethod(BHttpMethod::Post);
	request.SetFields(fields);

	// Authentication via Basic Authentication
	// The other way would be to obtain a token and then use the Token Bearer
	// header.
	if (((flags & NEEDS_AUTHORIZATION) != 0) && credentials.IsValid())
		request.SetAuthentication({credentials.Nickname(), credentials.PasswordClear()});

	auto replyData = make_exclusive_borrow<BMallocIO>();
	auto result =
		ServerHelper::GetHttpSession().Execute(std::move(request), BBorrow<BDataIO>(replyData));

	try {
		auto status = result.Status();
		HDDEBUG("%s; did receive http-status [%" B_PRId16 "] from [%s]",
			PROTOCOL_NAME, status.code, url.UrlString().String());

		switch (status.StatusCode()) {
			case BHttpStatusCode::Ok:
				result.Body(); // wait for the rest of the body
				break;

			case BHttpStatusCode::PreconditionFailed:
				ServerHelper::NotifyClientTooOld(result.Fields());
				return HD_CLIENT_TOO_OLD;

			default:
				HDERROR("%s; request to endpoint [.../%s] failed with http "
					"status [%" B_PRId16 "]\n", PROTOCOL_NAME, urlPathComponents,
					status.code);
				return B_ERROR;
		}
	} catch (const BNetworkRequestError& error) {
		HDERROR("%s; request to endpoint [.../%s] failed due to error: %s",
			PROTOCOL_NAME, urlPathComponents, error.DebugMessage().String());
		return B_ERROR;
	}

	try {
	replyData->Seek(0, SEEK_SET);
	} catch (const BNetworkRequestError& error) {
		HDERROR("%s; request to endpoint [.../%s] failed due to error: %s",
			PROTOCOL_NAME, urlPathComponents, error.DebugMessage().String());
		return B_ERROR;
	}

	if (Logger::IsTraceEnabled()) {
		HDLOGPREFIX(LOG_LEVEL_TRACE)
		printf("%s; response; ", PROTOCOL_NAME);
		_LogPayload(&*replyData, replyData->BufferLength());
		printf("\n");
	}

	BJsonMessageWriter jsonMessageWriter(reply);
	BJson::Parse(&*replyData, &jsonMessageWriter);
	status_t status = jsonMessageWriter.ErrorStatus();

	if (Logger::IsTraceEnabled() && status == B_BAD_DATA) {
		BString resultString(static_cast<const char *>(replyData->Buffer()),
			replyData->BufferLength());
		HDERROR("Parser choked on JSON:\n%s", resultString.String());
	}
	return status;
}


status_t
WebAppInterface::_SendJsonRequest(const char* urlPathComponents,
	const BString& jsonString, uint32 flags, BMessage& reply) const
{
	auto data = std::make_unique<BMemoryIO>(jsonString.String(), jsonString.Length() -1);

	return _SendJsonRequest(urlPathComponents, std::move(data), jsonString.Length() - 1,
		flags, reply);
}


status_t
WebAppInterface::_SendRawGetRequest(const BString urlPathComponents,
	BBorrow<BDataIO> stream)
{
	BUrl url = ServerSettings::CreateFullUrl(urlPathComponents);
	auto result = _SendRawGetRequestAsync(url, std::move(stream), BMessenger());
	try {
		auto status = result.Status();

		HDDEBUG("http-get; did receive http-status [%" B_PRId32 "] from [%s]",
			status.code, url.UrlString().String());

		if (status.StatusCode() == BHttpStatusCode::Ok) {
			// wait for the rest of the body
			result.Body();
			return B_OK;
		}

		HDERROR("failed to get data from '%s': %" B_PRIi32 "",
			url.UrlString().String(), status.code);
		return B_ERROR;
	} catch (const BNetworkRequestError& error) {
		HDERROR("failed to get data from '%s': %s",
			url.UrlString().String(), error.DebugMessage().String());
		return B_ERROR;
	}
}


BHttpResult
WebAppInterface::_SendRawGetRequestAsync(const BUrl& url,
	BBorrow<BDataIO> stream, const BMessenger& target)
{
	HDDEBUG("http-get; will make request to [%s]",
		url.UrlString().String());

	BHttpFields fields;
	ServerSettings::AugmentHeaders(fields);

	BHttpRequest request(url);
	request.SetFields(fields);

	return ServerHelper::GetHttpSession().Execute(std::move(request), std::move(stream), target);
}


void
WebAppInterface::_LogPayload(BPositionIO* requestData, size_t size)
{
	off_t requestDataOffset = requestData->Position();
	char buffer[LOG_PAYLOAD_LIMIT];

	if (size > LOG_PAYLOAD_LIMIT)
		size = LOG_PAYLOAD_LIMIT;

	if (B_OK != requestData->ReadExactly(buffer, size)) {
		printf("%s; error logging payload", PROTOCOL_NAME);
	} else {
		for (uint32 i = 0; i < size; i++) {
    		bool esc = buffer[i] > 126 ||
    			(buffer[i] < 0x20 && buffer[i] != 0x0a);

    		if (esc)
    			printf("\\u%02x", buffer[i]);
    		else
    			putchar(buffer[i]);
    	}

    	if (size == LOG_PAYLOAD_LIMIT)
    		printf("...(continues)");
	}

	requestData->Seek(requestDataOffset, SEEK_SET);
}


/*!	This will get the position of the data to get the length an then sets the
	offset to zero so that it can be re-read for reading the payload in to log
	or send.
*/

off_t
WebAppInterface::_LengthAndSeekToZero(BPositionIO* data)
{
	off_t dataSize = data->Position();
    data->Seek(0, SEEK_SET);
    return dataSize;
}
