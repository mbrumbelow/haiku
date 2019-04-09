/*
 * Copyright 2014, Dancsó Róbert <dancso.robert@d-rendszer.hu>
 *
 * Distributed under terms of the MIT license.
 */


#include "EncryptionUtils.h"

#include <Catalog.h>
#include <File.h>
#include <Locale.h>
#include <String.h>


const char*
EncryptionType(const char* path)
{
	char buffer[16];
	BString encrypter;
	BFile(path, B_READ_ONLY).ReadAt(0, &buffer, 11);
	encrypter.Append(buffer);

	if (encrypter.FindFirst("-FVE-FS-") >= 0) {
		return B_TRANSLATE("BitLocker encrypted");
	} else if (encrypter.FindFirst("PGPGUARD") >= 0) {
		return B_TRANSLATE("PGP encrypted");
	} else if (encrypter.FindFirst("SafeBoot") >= 0) {
		return B_TRANSLATE("SafeBoot encrypted");
	} else if (encrypter.FindFirst("LUKS") >= 0) {
		return B_TRANSLATE("LUKS encrypted");
	}

	return NULL;
}


