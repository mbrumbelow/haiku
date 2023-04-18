/*
 * Copyright 2011-2023, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 *		Alexander von Gluck IV <kallisti5@unixzen.com>
 */


#include <package/ValidateSignatureJob.h>

#include "Minisign.h"
#include <File.h>

#include <package/Context.h>


namespace BPackageKit {

namespace BPrivate {


ValidateSignatureJob::ValidateSignatureJob(const BContext& context,
	const BString& title, BEntry* signatureFile, BEntry* targetFile,
	bool failIfSignatureUntrusted)
	:
	inherited(context, title),
	fSignatureFile(signatureFile),
	fTargetFile(targetFile),
	fFailIfSignatureUntrusted(failIfSignatureUntrusted),
	fTargetTrusted(false)
{
}


ValidateSignatureJob::~ValidateSignatureJob()
{
}


status_t
ValidateSignatureJob::ValidateFromTrustPath(directory_which trustPathDirectory)
{
	BPath trustDB;
	status_t result = find_directory(B_SYSTEM_DATA_DIRECTORY, &trustDB);
	if (result != B_OK)
		return result;
	if ((result = trustDB.Append("trust_db")) != B_OK)
		return result;

	MinisignError* signError;
	MinisignPubkeyStruct* pubkey = minisign_pubkey_load(pk_file, NULL, signError);

	// Load Error, next key

	// Load Success, Verify
	// Verify Success notify
	fTargetTrusted = true;

	// Verify fail

	return result;
}


status_t
ValidateSignatureJob::Execute()
{
	if (fSignatureFile == NULL || fTargetFile == NULL)
		return B_BAD_VALUE;

	// Check the system TrustDB for matching trusted signers
	status_t result = ValidateFromTrustPath(B_SYSTEM_DATA_DIRECTORY);
	if (result != B_OK) {
		BString error = BString("Signature error: Unable to scan system trust_db\n");
		SetErrorString(error);
		return result;
	}

	// if we were successful in scanning the system trust db, however
	// the repo couldn't be validated... check user trust dbs
	if (!TrustedFile()) {
		ValidateFromTrustPath(B_USER_DATA_DIRECTORY);
		// We ignore errors here as user trust_db may not exist
	}

	if (!TrustedFile() && fFailIfSignatureUntrusted) {
		BString error = BString("Signature error: Unable to validate target file\n");
		SetErrorString(error);
		return B_BAD_DATA;
	}

	return B_OK;
}


bool
ValidateSignatureJob::TrustedFile() const
{
	return fTargetTrusted;
}


}	// namespace BPrivate

}	// namespace BPackageKit
