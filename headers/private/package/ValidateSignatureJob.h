/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Copyright 2023, Alexander von Gluck IV <kallisti5@unixzen.com>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__PRIVATE__VALIDATE_SIGNATURE_JOB_H_
#define _PACKAGE__PRIVATE__VALIDATE_SIGNATURE_JOB_H_


#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <String.h>

#include <package/Job.h>


namespace BPackageKit {

namespace BPrivate {


class ValidateSignatureJob : public BJob {
	typedef	BJob				inherited;

public:
								ValidateSignatureJob(
									const BContext& context,
									const BString& title,
									BEntry* signatureFile,
									BEntry* targetFile,
									bool failIfSignatureUntrusted = true);
	virtual						~ValidateSignatureJob();

			status_t			ValidateFromTrustPath(directory_which trustPathDirectory);

			bool				TrustedFile() const;

protected:
	virtual	status_t			Execute();

private:
			BEntry*				fSignatureFile;
			BEntry*				fTargetFile;
			bool				fFailIfSignatureUntrusted;

			bool				fTargetTrusted;
};


}	// namespace BPrivate

}	// namespace BPackageKit


#endif // _PACKAGE__PRIVATE__VALIDATE_SIGNATURE_JOB_H_
