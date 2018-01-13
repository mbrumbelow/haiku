/*
 * Copyright 2009-2010 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "PlaylistItem.h"

#include <stdio.h>

#include <Catalog.h>
#include <Locale.h>
#include <PropertyInfo.h>

#include "AudioTrackSupplier.h"
#include "TrackSupplier.h"
#include "VideoTrackSupplier.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MediaPlayer-PlaylistItem"


static property_info sProperties[] = {
	{ "Name", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Gets the name.", 0,
		{ B_STRING_TYPE }
	},
	{ "Author", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Gets the author.", 0,
		{ B_STRING_TYPE }
	},
	{ "Album", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Gets the album.", 0,
		{ B_STRING_TYPE }
	},
	{ "Title", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Gets the title.", 0,
		{ B_STRING_TYPE }
	},
	{ "TrackNumber", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Gets the track number.", 0,
		{ B_INT32_TYPE }
	},
	{ "Duration", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Gets the duration.", 0,
		{ B_INT64_TYPE }
	},
	{ "LocationURI", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Gets the location URI.", 0,
		{ B_STRING_TYPE }
	},

	{ 0 }
};


BHandler*
PlaylistItem::ResolveSpecifier(BMessage* msg, int32 index, BMessage* specifier,
	int32 what, const char* property)
{
	BPropertyInfo propertyInfo(sProperties);
	int32 i = propertyInfo.FindMatch(msg, index, specifier, what, property);

	if (i >= 0 && i < propertyInfo.CountProperties())
		return this;

	return BHandler::ResolveSpecifier(msg, index, specifier, what, property);
}


status_t
PlaylistItem::GetSupportedSuites(BMessage* msg)
{
	msg->AddString("suites", "suite/vnd.Haiku-MediaPlayer");

	BPropertyInfo propertyInfo(sProperties);
	msg->AddFlat("messages", &propertyInfo);

	return BHandler::GetSupportedSuites(msg);
}


void
PlaylistItem::MessageReceived(BMessage* msg)
{
	if (msg->what != B_GET_PROPERTY) {
		BHandler::MessageReceived(msg);
		return;
	}

	int32 index;
	BMessage specifier;
	int32 what;
	const char* property;

	if (msg->GetCurrentSpecifier(&index, &specifier, &what, &property)
		!= B_OK) {
		BHandler::MessageReceived(msg);
		return;
	}

	BPropertyInfo propertyInfo(sProperties);
	int32 propertyIndex = propertyInfo.FindMatch(msg, index, &specifier, what,
							property);

	if (propertyIndex == B_ERROR) {
		BHandler::MessageReceived(msg);
		return;
	}

	status_t rc = B_BAD_SCRIPT_SYNTAX;
	BMessage reply(B_REPLY);

	switch (propertyIndex) {
		case 0:
			rc = reply.AddString("result", Name());
			break;
		case 1:
			rc = reply.AddString("result", Author());
			break;
		case 2:
			rc = reply.AddString("result", Album());
			break;
		case 3:
			rc = reply.AddString("result", Title());
			break;
		case 4:
			rc = reply.AddInt32("result", TrackNumber());
			break;
		case 5:
			rc = reply.AddInt64("result", Duration());
			break;
		case 6:
			rc = reply.AddString("result", LocationURI());
			break;
		default:
			BHandler::MessageReceived(msg);
			return;
	}

	if (rc != B_OK) {
		reply.what = B_MESSAGE_NOT_UNDERSTOOD;
		reply.AddString("message", rc == B_BAD_SCRIPT_SYNTAX
									? "Didn't understand the specifier"
									: strerror(rc));
		reply.AddInt32("error", rc);
	}

	msg->SendReply(&reply);
}


PlaylistItem::Listener::Listener()
{
}

PlaylistItem::Listener::~Listener()
{
}

void PlaylistItem::Listener::ItemChanged(const PlaylistItem* item)
{
}


// #pragma mark -


//#define DEBUG_INSTANCE_COUNT
#ifdef DEBUG_INSTANCE_COUNT
static vint32 sInstanceCount = 0;
#endif


PlaylistItem::PlaylistItem()
	:
	fPlaybackFailed(false),
	fTrackSupplier(NULL)
{
#ifdef DEBUG_INSTANCE_COUNT
	atomic_add(&sInstanceCount, 1);
	printf("%p->PlaylistItem::PlaylistItem() (%ld)\n", this, sInstanceCount);
#endif
}


PlaylistItem::~PlaylistItem()
{
#ifdef DEBUG_INSTANCE_COUNT
	atomic_add(&sInstanceCount, -1);
	printf("%p->PlaylistItem::~PlaylistItem() (%ld)\n", this, sInstanceCount);
#endif
}


TrackSupplier*
PlaylistItem::GetTrackSupplier()
{
	if (fTrackSupplier == NULL)
		fTrackSupplier = _CreateTrackSupplier();

	return fTrackSupplier;
}


void
PlaylistItem::ReleaseTrackSupplier()
{
	delete fTrackSupplier;
	fTrackSupplier = NULL;
}


bool
PlaylistItem::HasTrackSupplier() const
{
	return fTrackSupplier != NULL;
}


BString
PlaylistItem::Name() const
{
	BString name;
	if (GetAttribute(ATTR_STRING_NAME, name) != B_OK)
		name = B_TRANSLATE_CONTEXT("<unnamed>", "PlaylistItem-name");
	return name;
}


BString
PlaylistItem::Author() const
{
	BString author;
	if (GetAttribute(ATTR_STRING_AUTHOR, author) != B_OK)
		author = B_TRANSLATE_CONTEXT("<unknown>", "PlaylistItem-author");
	return author;
}


BString
PlaylistItem::Album() const
{
	BString album;
	if (GetAttribute(ATTR_STRING_ALBUM, album) != B_OK)
		album = B_TRANSLATE_CONTEXT("<unknown>", "PlaylistItem-album");
	return album;
}


BString
PlaylistItem::Title() const
{
	BString title;
	if (GetAttribute(ATTR_STRING_TITLE, title) != B_OK)
		title = B_TRANSLATE_CONTEXT("<untitled>", "PlaylistItem-title");
	return title;
}


int32
PlaylistItem::TrackNumber() const
{
	int32 trackNumber;
	if (GetAttribute(ATTR_INT32_TRACK, trackNumber) != B_OK)
		trackNumber = 0;
	return trackNumber;
}


bigtime_t
PlaylistItem::Duration()
{
	bigtime_t duration;
	if (GetAttribute(ATTR_INT64_DURATION, duration) != B_OK) {
		duration = this->_CalculateDuration();
		SetAttribute(ATTR_INT64_DURATION, duration);
	}

	return duration;
}


void
PlaylistItem::SetPlaybackFailed()
{
	fPlaybackFailed = true;
}


//! You must hold the Playlist lock.
bool
PlaylistItem::AddListener(Listener* listener)
{
	if (listener && !fListeners.HasItem(listener))
		return fListeners.AddItem(listener);
	return false;
}


//! You must hold the Playlist lock.
void
PlaylistItem::RemoveListener(Listener* listener)
{
	fListeners.RemoveItem(listener);
}


void
PlaylistItem::_NotifyListeners() const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		Listener* listener = (Listener*)listeners.ItemAtFast(i);
		listener->ItemChanged(this);
	}
}


bigtime_t PlaylistItem::_CalculateDuration()
{
	// To be overridden in subclasses with more efficient methods
	TrackSupplier* supplier = GetTrackSupplier();

	AudioTrackSupplier* au = supplier->CreateAudioTrackForIndex(0);
	VideoTrackSupplier* vi = supplier->CreateVideoTrackForIndex(0);

	bigtime_t duration = max_c(au == NULL ? 0 : au->Duration(),
		vi == NULL ? 0 : vi->Duration());

	delete vi;
	delete au;
	ReleaseTrackSupplier();

	return duration;
}

