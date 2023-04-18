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
	fValidated(false)
{
}


ValidateSignatureJob::~ValidateSignatureJob()
{
}


status_t
ValidateSignatureJob::Execute()
{
	if (fSignatureFile == NULL || fTargetFile == NULL)
		return B_BAD_VALUE;

	status_t result = fExpectedChecksumAccessor->GetChecksum(expectedChecksum);
	if (result != B_OK)
		return result;

	result = fRealChecksumAccessor->GetChecksum(realChecksum);
	if (result != B_OK)
		return result;

	fChecksumsMatch = expectedChecksum.ICompare(realChecksum) == 0;

	if (fFailIfChecksumsDontMatch && !fChecksumsMatch) {
		BString error = BString("Signature error:\n")
			<< "expected '"	<< expectedChecksum << "'\n"
			<< "got      '" << realChecksum << "'";
		SetErrorString(error);
		return B_BAD_DATA;
	}

	return B_OK;
}


bool
ValidateSignatureJob::ChecksumsMatch() const
{
	return fChecksumsMatch;
}


}	// namespace BPrivate

}	// namespace BPackageKit
