/*
 * Copyright 2001-2017 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		Axel Dörfler
 *		Jeremy Rand, jrand@magma.ca
 *		John Scipione, jscipione@gmail.com
 */


#include <Deskbar.h>

#include <string.h>

#include <InterfaceDefs.h>
#include <Messenger.h>
#include <Message.h>
#include <Node.h>
#include <Rect.h>
#include <View.h>


// ToDo: in case the BDeskbar methods are called from a Deskbar add-on,
//	they will currently deadlock most of the time (only those that do
//	not need a reply will work).
//	That should be fixed in the Deskbar itself, even if the Be API found
//	a way around that (that doesn't work too well, BTW)

// The API in this file should be considered as part of OpenTracker - but
// should work with all versions of Tracker available for Haiku.


static const char* kDeskbarSignature = "application/x-vnd.Be-TSKB";

static const uint32 kMsgIsAlwaysOnTop = 'gtop';
static const uint32 kMsgAlwaysOnTop = 'stop';
static const uint32 kMsgIsAutoRaise = 'grse';
static const uint32 kMsgAutoRaise = 'srse';
static const uint32 kMsgIsAutoHide = 'ghid';
static const uint32 kMsgAutoHide = 'shid';
static const uint32 kMsgGetRecentCounts = 'grct';
static const uint32 kMsgSetRecentCounts = 'srct';

static const uint32 kMsgAddView = 'icon';
static const uint32 kMsgAddAddOn = 'adon';
static const uint32 kMsgHasItem = 'exst';
static const uint32 kMsgGetItemInfo = 'info';
static const uint32 kMsgCountItems = 'cwnt';
static const uint32 kMsgRemoveItem = 'remv';
static const uint32 kMsgLocation = 'gloc';
static const uint32 kMsgIsExpanded = 'gexp';
static const uint32 kMsgSetLocation = 'sloc';
static const uint32 kMsgExpand = 'sexp';


status_t
get_deskbar_frame(BRect* frame)
{
	BMessenger deskbar(kDeskbarSignature);

	status_t result;

	BMessage request(B_GET_PROPERTY);
	request.AddSpecifier("Frame");
	request.AddSpecifier("Window", "Deskbar");

	BMessage reply;
	result = deskbar.SendMessage(&request, &reply);
	if (result == B_OK)
		result = reply.FindRect("result", frame);

	return result;
}


//	#pragma mark - BDeskbar


BDeskbar::BDeskbar()
	:
	fMessenger(new BMessenger(kDeskbarSignature))
{
}


BDeskbar::~BDeskbar()
{
	delete fMessenger;
}


bool
BDeskbar::IsRunning() const
{
	return fMessenger->IsValid();
}


//	#pragma mark - Item querying methods


BRect
BDeskbar::Frame() const
{
	BRect frame(0.0, 0.0, 0.0, 0.0);
	get_deskbar_frame(&frame);

	return frame;
}


deskbar_location
BDeskbar::Location(bool* _isExpanded) const
{
	deskbar_location location = B_DESKBAR_RIGHT_TOP;
	BMessage request(kMsgLocation);
	BMessage reply;

	if (_isExpanded)
		*_isExpanded = true;

	if (fMessenger->IsTargetLocal()) {
		// ToDo: do something about this!
		// (if we just ask the Deskbar in this case, we would deadlock)
		return location;
	}

	if (fMessenger->SendMessage(&request, &reply) == B_OK) {
		int32 value;
		if (reply.FindInt32("location", &value) == B_OK)
			location = static_cast<deskbar_location>(value);

		if (_isExpanded && reply.FindBool("expanded", _isExpanded) != B_OK)
			*_isExpanded = true;
	}

	return location;
}


status_t
BDeskbar::SetLocation(deskbar_location location, bool expanded)
{
	BMessage request(kMsgSetLocation);
	request.AddInt32("location", static_cast<int32>(location));
	request.AddBool("expand", expanded);

	return fMessenger->SendMessage(&request);
}


//	#pragma mark - Other state methods


bool
BDeskbar::IsExpanded() const
{
	BMessage request(kMsgIsExpanded);
	BMessage reply;
	bool isExpanded = true;

	if (fMessenger->SendMessage(&request, &reply) == B_OK)
		reply.FindBool("expanded", &isExpanded);

	return isExpanded;
}


status_t
BDeskbar::Expand(bool expand)
{
	BMessage request(kMsgExpand);
	request.AddBool("expand", expand);

	return fMessenger->SendMessage(&request);
}


bool
BDeskbar::IsAlwaysOnTop() const
{
	BMessage request(kMsgIsAlwaysOnTop);
	BMessage reply;
	bool isAlwaysOnTop = false;

	if (fMessenger->SendMessage(&request, &reply) == B_OK)
		reply.FindBool("always on top", &isAlwaysOnTop);

	return isAlwaysOnTop;
}


status_t
BDeskbar::SetAlwaysOnTop(bool alwaysOnTop)
{
	BMessage request(kMsgAlwaysOnTop);
	request.AddBool("always on top", alwaysOnTop);

	return fMessenger->SendMessage(&request);
}


bool
BDeskbar::IsAutoRaise() const
{
	BMessage request(kMsgIsAutoRaise);
	BMessage reply;
	bool isAutoRaise = false;

	if (fMessenger->SendMessage(&request, &reply) == B_OK)
		reply.FindBool("auto raise", &isAutoRaise);

	return isAutoRaise;
}


status_t
BDeskbar::SetAutoRaise(bool autoRaise)
{
	BMessage request(kMsgAutoRaise);
	request.AddBool("auto raise", autoRaise);

	return fMessenger->SendMessage(&request);
}


bool
BDeskbar::IsAutoHide() const
{
	BMessage request(kMsgIsAutoHide);
	BMessage reply;
	bool isAutoHidden = false;

	if (fMessenger->SendMessage(&request, &reply) == B_OK)
		reply.FindBool("auto hide", &isAutoHidden);

	return isAutoHidden;
}


status_t
BDeskbar::SetAutoHide(bool autoHide)
{
	BMessage request(kMsgAutoHide);
	request.AddBool("auto hide", autoHide);

	return fMessenger->SendMessage(&request);
}


//	#pragma mark - Recent count methods


status_t
BDeskbar::GetRecentCounts(int32* documents, int32* applications, int32* folders)
{
	BMessage request(kMsgGetRecentCounts);
	BMessage reply;

	status_t result = fMessenger->SendMessage(&request, &reply);
	if (result != B_OK)
		return result;

	if (documents != NULL)
		result = reply.FindInt32("documents", documents);

	if (applications != NULL)
		result = reply.FindInt32("applications", applications);

	if (folders != NULL)
		result = reply.FindInt32("folders", folders);

	return result;
}


int32
BDeskbar::RecentDocumentsCount()
{
	int32 documents = 10;
	if (GetRecentCounts(&documents, NULL, NULL) == B_OK)
		return documents;

	return -1;
}


int32
BDeskbar::RecentApplicationsCount()
{
	int32 applications = 10;
	if (GetRecentCounts(NULL, &applications, NULL) == B_OK)
		return applications;

	return -1;
}


int32
BDeskbar::RecentFoldersCount()
{
	int32 folders = 10;
	if (GetRecentCounts(NULL, NULL, &folders) == B_OK)
		return folders;

	return -1;
}


status_t
BDeskbar::SetRecentCounts(int32 applications, int32 documents, int32 folders)
{
	BMessage request(kMsgSetRecentCounts);

	if (documents >= 0)
		request.AddInt32("documents", documents);

	if (applications >= 0)
		request.AddInt32("applications", applications);

	if (folders >= 0)
		request.AddInt32("folders", folders);

	return fMessenger->SendMessage(&request);
}


status_t
BDeskbar::SetRecentDocumentsCount(int32 documents)
{
	return SetRecentCounts(documents, -1, -1);
}


status_t
BDeskbar::SetRecentApplicationsCount(int32 applications)
{
	return SetRecentCounts(-1, applications, -1);
}


status_t
BDeskbar::SetRecentFoldersCount(int32 folders)
{
	return SetRecentCounts(-1, -1, folders);
}


//	#pragma mark - Item querying methods


status_t
BDeskbar::GetItemInfo(int32 id, const char** _name) const
{
	if (_name == NULL)
		return B_BAD_VALUE;

	// Note: Be's implementation returns B_BAD_VALUE if *_name was NULL,
	// not just if _name was NULL.  This doesn't make much sense, so we
	// do not imitate this behaviour.

	BMessage request(kMsgGetItemInfo);
	request.AddInt32("id", id);

	BMessage reply;
	status_t result = fMessenger->SendMessage(&request, &reply);
	if (result == B_OK) {
		const char* name;
		result = reply.FindString("name", &name);
		if (result == B_OK) {
			*_name = strdup(name);
			if (*_name == NULL)
				result = B_NO_MEMORY;
		}
	}

	return result;
}


status_t
BDeskbar::GetItemInfo(const char* name, int32* _id) const
{
	if (name == NULL)
		return B_BAD_VALUE;

	BMessage request(kMsgGetItemInfo);
	request.AddString("name", name);

	BMessage reply;
	status_t result = fMessenger->SendMessage(&request, &reply);
	if (result == B_OK)
		result = reply.FindInt32("id", _id);

	return result;
}


bool
BDeskbar::HasItem(int32 id) const
{
	BMessage request(kMsgHasItem);
	request.AddInt32("id", id);

	BMessage reply;
	if (fMessenger->SendMessage(&request, &reply) == B_OK)
		return reply.FindBool("exists");

	return false;
}


bool
BDeskbar::HasItem(const char* name) const
{
	BMessage request(kMsgHasItem);
	request.AddString("name", name);

	BMessage reply;
	if (fMessenger->SendMessage(&request, &reply) == B_OK)
		return reply.FindBool("exists");

	return false;
}


uint32
BDeskbar::CountItems() const
{
	BMessage request(kMsgCountItems);	
	BMessage reply;

	if (fMessenger->SendMessage(&request, &reply) == B_OK)
		return reply.FindInt32("count");

	return 0;
}


//	#pragma mark - Item querying methods


status_t
BDeskbar::AddItem(BView* view, int32* _id)
{
	BMessage archive;
	status_t result = view->Archive(&archive);
	if (result < B_OK)
		return result;

	BMessage request(kMsgAddView);
	request.AddMessage("view", &archive);

	BMessage reply;
	result = fMessenger->SendMessage(&request, &reply);
	if (result == B_OK) {
		if (_id != NULL)
			result = reply.FindInt32("id", _id);
		else
			reply.FindInt32("error", &result);
	}

	return result;
}


status_t
BDeskbar::AddItem(entry_ref* addon, int32* _id)
{
	BMessage request(kMsgAddAddOn);
	request.AddRef("addon", addon);

	BMessage reply;
	status_t status = fMessenger->SendMessage(&request, &reply);
	if (status == B_OK) {
		if (_id != NULL)
			status = reply.FindInt32("id", _id);
		else
			reply.FindInt32("error", &status);
	}

	return status;
}


status_t
BDeskbar::RemoveItem(int32 id)
{
	BMessage request(kMsgRemoveItem);
	request.AddInt32("id", id);

	// ToDo: the Deskbar does not reply to this message, so we don't
	// know if it really succeeded - we can just acknowledge that the
	// message was sent to the Deskbar

	return fMessenger->SendMessage(&request);
}


status_t
BDeskbar::RemoveItem(const char* name)
{
	BMessage request(kMsgRemoveItem);
	request.AddString("name", name);

	// ToDo: the Deskbar does not reply to this message, so we don't
	// know if it really succeeded - we can just acknowledge that the
	// message was sent to the Deskbar

	return fMessenger->SendMessage(&request);
}
