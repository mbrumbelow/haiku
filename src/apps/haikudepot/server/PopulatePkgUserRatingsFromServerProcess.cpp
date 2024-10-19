/*
 * Copyright 2016-2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Note that this file included code earlier from `Model.cpp` and
 * copyrights have been latterly been carried across in 2024.
 */


#include "PopulatePkgUserRatingsFromServerProcess.h"

#include <Autolock.h>
#include <Catalog.h>

#include "Logger.h"
#include "ServerHelper.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PopulatePkgUserRatingsFromServerProcess"


PopulatePkgUserRatingsFromServerProcess::PopulatePkgUserRatingsFromServerProcess(
	PackageInfoRef packageInfo, Model* model)
	:
	fModel(model),
	fPackageInfo(packageInfo)
{
}


PopulatePkgUserRatingsFromServerProcess::~PopulatePkgUserRatingsFromServerProcess()
{
}


const char*
PopulatePkgUserRatingsFromServerProcess::Name() const
{
	return "PopulatePkgUserRatingsFromServerProcess";
}


const char*
PopulatePkgUserRatingsFromServerProcess::Description() const
{
	return B_TRANSLATE("Fetching user ratings for package");
}


status_t
PopulatePkgUserRatingsFromServerProcess::RunInternal()
{
	// TODO; use API spec to code generation techniques instead of this manually written client.

	status_t status = B_OK;

	// Retrieve info from web-app
	BMessage info;

	BString packageName;
	BString webAppRepositoryCode;

	{
		BAutolock locker(&fLock);
		packageName = fPackageInfo->Name();
		const DepotInfo* depot = fModel->DepotForName(fPackageInfo->DepotName());

		if (depot != NULL)
			webAppRepositoryCode = depot->WebAppRepositoryCode();
	}

	if (status == B_OK) {
		status = fModel->GetWebAppInterface()->RetrieveUserRatingsForPackageForDisplay(packageName,
			webAppRepositoryCode, BString(), 0, PACKAGE_INFO_MAX_USER_RATINGS, info);
			// ^ note intentionally not using the repository source code as this would then show
			// too few results as it would be architecture specific.
	}

	UserRatingInfoRef userRatingInfo(new UserRatingInfo(), true);

	if (status == B_OK) {
		// Parse message
		BMessage result;
		BMessage items;

		status = info.FindMessage("result", &result);

		if (status == B_OK) {

			if (result.FindMessage("items", &items) == B_OK) {

				int32 index = 0;
				while (true) {
					BString name;
					name << index++;

					BMessage item;
					if (items.FindMessage(name, &item) != B_OK)
						break;

					BString code;
					if (item.FindString("code", &code) != B_OK) {
						HDERROR("corrupt user rating at index %" B_PRIi32, index);
						continue;
					}

					BString user;
					BMessage userInfo;
					if (item.FindMessage("user", &userInfo) != B_OK
							|| userInfo.FindString("nickname", &user) != B_OK) {
						HDERROR("ignored user rating [%s] without a user nickname", code.String());
						continue;
					}

					// Extract basic info, all items are optional
					BString languageCode;
					BString comment;
					double rating;
					item.FindString("naturalLanguageCode", &languageCode);
					item.FindString("comment", &comment);
					if (item.FindDouble("rating", &rating) != B_OK)
						rating = -1;
					if (comment.Length() == 0 && rating == -1) {
						HDERROR("rating [%s] has no comment or rating so will"
								" be ignored",
							code.String());
						continue;
					}

					// For which version of the package was the rating?
					BString major = "?";
					BString minor = "?";
					BString micro = "";
					double revision = -1;
					BString architectureCode = "";
					BMessage version;
					if (item.FindMessage("pkgVersion", &version) == B_OK) {
						version.FindString("major", &major);
						version.FindString("minor", &minor);
						version.FindString("micro", &micro);
						version.FindDouble("revision", &revision);
						version.FindString("architectureCode", &architectureCode);
					}
					BString versionString = major;
					versionString << ".";
					versionString << minor;
					if (!micro.IsEmpty()) {
						versionString << ".";
						versionString << micro;
					}
					if (revision > 0) {
						versionString << "-";
						versionString << (int)revision;
					}

					if (!architectureCode.IsEmpty()) {
						versionString << " " << STR_MDASH << " ";
						versionString << architectureCode;
					}

					double createTimestamp;
					item.FindDouble("createTimestamp", &createTimestamp);

					// Add the rating to the PackageInfo
					UserRatingRef userRating(
						new UserRating(UserInfo(user), rating, comment, languageCode,
							// note that language identifiers are "code" in HDS and "id" in Haiku
							versionString, (uint64) createTimestamp),
						true);
					userRatingInfo->AddUserRating(userRating);
					HDDEBUG("rating [%s] retrieved from server", code.String());
				}

				userRatingInfo->SetUserRatingsPopulated();
				HDDEBUG("did retrieve %" B_PRIi32 " user ratings for [%s]", index - 1,
					packageName.String());
			}
		} else {
			int32 errorCode = WebAppInterface::ErrorCodeFromResponse(info);

			if (errorCode != ERROR_CODE_NONE)
				ServerHelper::NotifyServerJsonRpcError(info);
		}
	} else {
		ServerHelper::NotifyTransportError(status);
	}

	// Now fetch the user rating summary which is derived separately as it is
	// not just based on the user-ratings downloaded; it is calculated according
	// to an algorithm. This is best executed server-side to avoid discrepancy.

	// TODO; re-enable this logic when HDS version 1.0.166 is released.
	/*
	BMessage summaryResponse;

	if (status == B_OK) {
		status = fModel->GetWebAppInterface()->RetrieveUserRatingSummaryForPackage(packageName,
			webAppRepositoryCode, summaryResponse);
	}

	if (status == B_OK) {
		// Parse message

		UserRatingSummaryRef userRatingSummary(new UserRatingSummary(), true);

		BMessage result;

		// TODO; this entire BMessage handling is historical and needs to be swapped out with
		//	generated code from the API spec; it just takes time unfortunately.

		status = summaryResponse.FindMessage("result", &result);

		double sampleSizeF;
		bool hasData;

		if (status == B_OK)
			status = result.FindDouble("sampleSize", &sampleSizeF);

		if (status == B_OK)
			userRatingSummary->SetRatingCount(static_cast<int>(sampleSizeF));

		hasData = status == B_OK && userRatingSummary->RatingCount() > 0;

		if (hasData) {
			double ratingF;

			if (status == B_OK)
				status = result.FindDouble("rating", &ratingF);

			if (status == B_OK)
				userRatingSummary->SetAverageRating(ratingF);
		}

		if (hasData) {
			BMessage ratingDistributionItems;
			BMessage item;

			status = result.FindMessage("ratingDistribution", &ratingDistributionItems);

			int32 index = 0;
			while (status == B_OK) {
				BString name;
				name << index++;

				BMessage ratingDistributionItem;
				if (ratingDistributionItems.FindMessage(name, &ratingDistributionItem) != B_OK)
					break;

				double ratingDistributionRatingF;

				if (status == B_OK) {
					status
						= ratingDistributionItem.FindDouble("rating", &ratingDistributionRatingF);
				}

				double ratingDistributionTotalF;

				if (status == B_OK)
					status = ratingDistributionItem.FindDouble("total", &ratingDistributionTotalF);

				userRatingSummary->SetRatingByStar(static_cast<int>(ratingDistributionRatingF),
					static_cast<int>(ratingDistributionTotalF));
			}

			userRatingInfo->SetSummary(userRatingSummary);
		} else {
			int32 errorCode = WebAppInterface::ErrorCodeFromResponse(summaryResponse);

			if (errorCode != ERROR_CODE_NONE)
				ServerHelper::NotifyServerJsonRpcError(summaryResponse);
		}
	} else {
		ServerHelper::NotifyTransportError(status);
	}
	*/

	if (status == B_OK) {
		// TODO; later make the PackageInfo immutable to avoid the need for locking here.
		BAutolock locker(&fLock);
		fPackageInfo->SetUserRatingInfo(userRatingInfo);
	}

	return status;
}
