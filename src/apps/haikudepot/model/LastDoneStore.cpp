/*
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "LastDoneStore.h"

#include <time.h>

#include <Autolock.h>
#include <FileIO.h>
#include <File.h>
#include <Json.h>
#include <JsonTextWriter.h>

#include "LastDonesJsonListener.h"
#include "StorageUtils.h"


#define MILLIS_DAY (1000 * 60 * 60 * 24);


LastDoneStore::LastDoneStore()
	:
	fLastDones(NULL)
{
}


LastDoneStore::~LastDoneStore()
{
	delete fLastDones;
}


status_t
LastDoneStore::Init(const BPath& path)
{
	delete fLastDones;
	bool exists;
	fPath = path;
	status_t result = B_OK;

	if (result == B_OK)
		result = StorageUtils::ExistsObject(path, &exists, NULL, NULL);

	if (exists) {
		SingleLastDonesJsonListener listener;
		BFile file(path.Path(), O_RDONLY);
		BPrivate::BJson::Parse(&file, &listener);
		result = listener.ErrorStatus();
		if (result == B_OK)
			fLastDones = listener.Target();
		else
			printf("unable to load the last-dones from [%s]\n", path.Path());
	} else {
		printf("the last-dones does not exist at [%s]\n", path.Path());
	}

	if (fLastDones == NULL) {
		fLastDones = new LastDones();
	}

	return result;
}


uint64
LastDoneStore::DaysSinceLastDone(const BString& type, const BString& key)
{
	BAutolock lock(fLock);
	uint64 now = (uint64) time(NULL) * 1000;
	uint64 then = _LastDone(type, key);
	if (now < then)
		return 0;
	return (now - then) / MILLIS_DAY;
}


status_t
LastDoneStore::MarkLastDone(const BString& type, const BString& key)
{
	BAutolock lock(fLock);
	if (fLastDones == NULL)
		return B_OK;
	LastDone* item = _FindOrCreateLastDone(type, key);
	item->SetDoneTimestamp((uint64) (time(NULL) * 1000));
		// to be consistent keep everything stored as millis
	return _Store();
}


uint64
LastDoneStore::_LastDone(const BString& type, const BString& key) const
{
	LastDone* item = _FindLastDone(type, key);
	if (item == NULL || item->DoneTimestampIsNull())
		return 0;
	return item->DoneTimestamp();
}


LastDone*
LastDoneStore::_FindOrCreateLastDone(
	const BString& type, const BString& key) const
{
	LastDone* item = _FindLastDone(type, key);

	if (item == NULL) {
		item = new LastDone();
		item->SetType(new BString(type));
		item->SetKey(new BString(key));
		fLastDones->AddToItems(item);
	}

	return item;
}


LastDone*
LastDoneStore::_FindLastDone(const BString& type, const BString& key) const
{
	if (fLastDones == NULL)
		return NULL;

	for (int32 i = 0; i < fLastDones->CountItems(); i++) {
		LastDone* item = fLastDones->ItemsItemAt(i);
		if (
			!item->KeyIsNull() && !item->TypeIsNull()
			&& *(item->Type()) == type
			&& *(item->Key()) == key) {
			return item;
		}
	}

	return NULL;
}


#define TMP_WRITE_IF_OK(EXP) if(result == B_OK) result = EXP

status_t
LastDoneStore::_Store() const
{
	// TODO; this writes to the file manually which would be better replaced by
	// a schema-based writer driven by the project's python schema --> C++
	// scripts.  Written here inline for expediencies sake to get B2 released,
	// but isn't very nice.

	BFile file(fPath.Path(), O_WRONLY | O_CREAT);
	BJsonTextWriter writer(&file);
	status_t result = B_OK;

	TMP_WRITE_IF_OK(writer.WriteObjectStart());
	TMP_WRITE_IF_OK(writer.WriteObjectName("items"));
	TMP_WRITE_IF_OK(writer.WriteArrayStart());

	for (int32 i = 0; i < fLastDones->CountItems(); i++) {
		LastDone* item = fLastDones->ItemsItemAt(i);

		TMP_WRITE_IF_OK(writer.WriteObjectStart());
		if (!item->KeyIsNull()) {
			TMP_WRITE_IF_OK(writer.WriteObjectName("key"));
			TMP_WRITE_IF_OK(writer.WriteString(item->Key()->String()));
		}
		if (!item->TypeIsNull()) {
			TMP_WRITE_IF_OK(writer.WriteObjectName("type"));
			TMP_WRITE_IF_OK(writer.WriteString(item->Type()->String()));
		}
		if (!item->DoneTimestampIsNull()) {
			TMP_WRITE_IF_OK(writer.WriteObjectName("doneTimestamp"));
			TMP_WRITE_IF_OK(writer.WriteInteger(item->DoneTimestamp()));
		}
		TMP_WRITE_IF_OK(writer.WriteObjectEnd());
	}

	TMP_WRITE_IF_OK(writer.WriteArrayEnd());
	TMP_WRITE_IF_OK(writer.WriteObjectEnd());

	if (result != B_OK)
		printf("! unable to write the last done store to [%s]\n", fPath.Path());

	return result;
}
