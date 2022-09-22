/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2022, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WEB_APP_INTERFACE_H
#define WEB_APP_INTERFACE_H

#include <Application.h>
#include <ExclusiveBorrow.h>
#include <JsonWriter.h>
#include <HttpResult.h>
#include <String.h>
#include <package/PackageVersion.h>

#include "PackageInfo.h"
#include "UserCredentials.h"
#include "UserDetail.h"
#include "UserUsageConditions.h"


class BDataIO;
class BMessage;
class BMessenger;
class BUrl;
using BPackageKit::BPackageVersion;
using BPrivate::Network::BBorrow;
using BPrivate::Network::BHttpResult;


/*! These are error codes that are sent back to the client from the server */

#define ERROR_CODE_NONE							0
#define ERROR_CODE_VALIDATION					-32800
#define ERROR_CODE_OBJECTNOTFOUND				-32801
#define ERROR_CODE_CAPTCHABADRESPONSE			-32802
#define ERROR_CODE_AUTHORIZATIONFAILURE			-32803
#define ERROR_CODE_BADPKGICON					-32804
#define ERROR_CODE_LIMITEXCEEDED				-32805
#define ERROR_CODE_AUTHORIZATIONRULECONFLICT	-32806

/*! This constant can be used to indicate the lack of a rating. */

#define RATING_NONE -1


class WebAppInterface {
public:
								WebAppInterface();
								WebAppInterface(const WebAppInterface& other);
	virtual						~WebAppInterface();

			WebAppInterface&	operator=(const WebAppInterface& other);

			void				SetAuthorization(const UserCredentials& value);
			const BString&		Nickname() const;

			status_t			GetChangelog(
									const BString& packageName,
									BMessage& message);

			status_t			RetreiveUserRatingsForPackageForDisplay(
									const BString& packageName,
									const BString& webAppRepositoryCode,
									const BString& webAppRepositorySourceCode,
									int resultOffset, int maxResults,
									BMessage& message);

			status_t			RetreiveUserRatingForPackageAndVersionByUser(
									const BString& packageName,
									const BPackageVersion& version,
									const BString& architecture,
									const BString& webAppRepositoryCode,
									const BString& webAppRepositorySourceCode,
									const BString& userNickname,
									BMessage& message);

			status_t			CreateUserRating(
									const BString& packageName,
									const BPackageVersion& version,
									const BString& architecture,
									const BString& webAppRepositoryCode,
									const BString& webAppRepositorySourceCode,
									const BString& languageCode,
									const BString& comment,
									const BString& stability,
									int rating,
									BMessage& message);

			status_t			UpdateUserRating(
									const BString& ratingID,
									const BString& languageCode,
									const BString& comment,
									const BString& stability,
									int rating, bool active,
									BMessage& message);

			status_t			RetrieveUserDetailForCredentials(
									const UserCredentials& credentials,
									BMessage& message);

			status_t			RetrieveCurrentUserDetail(
									BMessage& message);

			status_t			RetrieveUserUsageConditions(
									const BString& code,
									UserUsageConditions& conditions);

			status_t			AgreeUserUsageConditions(const BString& code,
									BMessage& responsePayload);

			status_t			RetrieveScreenshot(
									const BString& code,
									int32 width, int32 height,
									BBorrow<BDataIO> stream);

			BHttpResult			RetrieveScreenshotAsync(
									const BString& code,
									int32 width, int32 height,
									BBorrow<BDataIO> stream,
									const BMessenger& target);

			status_t			RequestCaptcha(BMessage& message);

			status_t			CreateUser(const BString& nickName,
									const BString& passwordClear,
									const BString& email,
									const BString& captchaToken,
									const BString& captchaResponse,
									const BString& languageCode,
									const BString& userUsageConditionsCode,
									BMessage& message);

			status_t			AuthenticateUser(const BString& nickName,
									const BString& passwordClear,
									BMessage& message);

			status_t			IncrementViewCounter(
									const PackageInfoRef package,
									const DepotInfoRef depot,
									BMessage& message);

	static	int32				ErrorCodeFromResponse(
									BMessage& responseEnvelopeMessage);

	static	status_t			UnpackUserDetail(
									BMessage& responseEnvelopeMessage,
									UserDetail& userDetail);
private:


			status_t			_RetrieveUserUsageConditionsMeta(
									const BString& code, BMessage& message);
			status_t			_RetrieveUserUsageConditionsCopy(
									const BString& code, BBorrow<BDataIO> stream);

			status_t			_SendJsonRequest(const char* urlPathComponents,
									const BString& jsonString, uint32 flags,
									BMessage& reply) const;
			status_t			_SendJsonRequest(const char* urlPathComponents,
									UserCredentials credentials,
									std::unique_ptr<BPositionIO> requestData,
									size_t requestDataSize, uint32 flags,
									BMessage& reply) const;
			status_t			_SendJsonRequest(const char* urlPathComponents,
									std::unique_ptr<BPositionIO> requestData,
									size_t requestDataSize, uint32 flags,
									BMessage& reply) const;

			status_t			_SendRawGetRequest(
									const BString urlPathComponents,
									BBorrow<BDataIO> stream);
			BHttpResult			_SendRawGetRequestAsync(
									const BUrl& url,
									BBorrow<BDataIO> stream,
									const BMessenger& target);
	static	void				_LogPayload(BPositionIO* requestData,
									size_t size);
	static	off_t				_LengthAndSeekToZero(BPositionIO* data);

private:
			UserCredentials		fCredentials;
	static	int					fRequestIndex;
};


#endif // WEB_APP_INTERFACE_H
