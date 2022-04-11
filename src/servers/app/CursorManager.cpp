/*
 * Copyright 2001-2016, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 */


/*!	Handles the system's cursor infrastructure */


#include "CursorManager.h"

#include "CursorData.h"
#include "ServerCursor.h"
#include "ServerConfig.h"
#include "ServerTokenSpace.h"

#include <Autolock.h>
#include <Directory.h>
#include <String.h>
#include <IconUtils.h>

#include <new>
#include <stdio.h>

#define IIR_GAUSS_BLUR_IMPLEMENTATION
#include "iir_gauss_blur.h"

// TODO: find a better way to handle this
// this should actually never happen
static
BBitmap
CreateFallbackCursor()
{
	BBitmap fallback(BRect(0, 0, 21, 21), B_BITMAP_NO_SERVER_LINK, B_RGBA32);
	fallback.SetBits(kCursorSystemDefaultBits, 22 * 22 * 4, 0, B_RGBA32);
	return fallback;
}

BBitmap
CursorManager::CreateCursorBitmap(uint32 size, const uint8* vector,
	uint32 vectorSize, float shadowStrength)
{
	const BRect rect(0, 0, size - 1, size - 1);
	const uint32 flags = B_BITMAP_NO_SERVER_LINK;

	BBitmap cursor(rect, flags, B_RGBA32);
	status_t status = BIconUtils::GetVectorIcon(vector, vectorSize, &cursor);
	if (status != B_OK)
		return CreateFallbackCursor();

	if (shadowStrength <= 0.0) {
		for (int32 i = 0; i < cursor.BitsLength(); i += 4) {
			uint8* bits = (uint8*)cursor.Bits() + i;
			// this produces smoother results than normal
			// premultiplication when there's no shadow
			// (no artifacting on white edges)
			if (bits[0] > bits[3])
				bits[0] = bits[3];
			if (bits[1] > bits[3])
				bits[1] = bits[3];
			if (bits[2] > bits[3])
				bits[2] = bits[3];
		}
		return cursor;
	}

	BBitmap shadow(rect, flags, B_RGBA32);
	memset(shadow.Bits(), 0, shadow.BitsLength());

	int32 offset = size / 32;
	if (offset == 0)
		offset = 1; // <32px cursors
	status = shadow.ImportBits(&cursor, BPoint(0, 0), BPoint(offset, offset),
		BSize(size - offset - 1, size - offset - 1));
	if (status != B_OK)
		return CreateFallbackCursor();

	iir_gauss_blur(size, size, 4, (uint8*)shadow.Bits(), 0.8);
	for (int32 i = 0; i < shadow.BitsLength(); i += 4) {
		uint8* bits = (uint8*)shadow.Bits() + i;
		bits[0] = 0;
		bits[1] = 0;
		bits[2] = 0;
		bits[3] *= shadowStrength;
	}

	BBitmap composite = BBitmap(rect, flags, B_RGBA32);

	uint8* s = (uint8*)shadow.Bits();
	uint8* c = (uint8*)cursor.Bits();
	uint8* d = (uint8*)composite.Bits();
	for (uint32 y = 0; y < size; y++) {
		for (uint32 x = 0; x < size; x++) {
			uint8 a = c[3] + (255 - c[3]) * (s[3] / 255.0);
			d[3] = a;
			for (int32 i = 0; i < 3; ++i) {
				d[i] = ((s[i] * (255 - c[3]) + 255) >> 8) + c[i];

				// premultiply
				d[i] = d[i] * int32(a) / 255.0;
			}
			s += 4;
			c += 4;
			d += 4;
		}
	}

	return composite;
}

CursorManager::CursorManager()
	:
	BLocker("CursorManager")
{
	InitVectorCursors();
}


//! Does all the teardown
CursorManager::~CursorManager()
{
	for (int32 i = 0; i < fCursorList.CountItems(); i++) {
		ServerCursor* cursor = ((ServerCursor*)fCursorList.ItemAtFast(i));
		cursor->fManager = NULL;
		cursor->ReleaseReference();
	}
}

static
BPoint
ScaleHotspot(BPoint hotspot)
{
	return BPoint(uint32((gCursorSize / 32.0) * hotspot.x),
		uint32((gCursorSize / 32.0) * hotspot.y));
}

void
CursorManager::InitVectorCursors()
{
	if (gCursorSize <= 0 || gCursorSize >= 256)
		return;

	// TODO: free old cursor memory

	const BPoint kHandHotspot(uint32(gCursorSize / 20),
		uint32(gCursorSize / 20));
	const BPoint kResizeHotspot = ScaleHotspot(BPoint(8, 8));

	// Init system cursors from vectors

	BBitmap bitmap = CreateCursorBitmap(gCursorSize,
		kCursorSystemDefaultBits, 3840, gCursorShadow / 10.0);
	_InitCursor(fCursorSystemDefault, (uint8*)bitmap.Bits(),
		B_CURSOR_ID_SYSTEM_DEFAULT, kHandHotspot);

	bitmap = CreateCursorBitmap(gCursorSize,
		kCursorContextMenuBits, 8815, gCursorShadow / 10.0);
	_InitCursor(fCursorContextMenu, (uint8*)bitmap.Bits(),
		B_CURSOR_ID_CONTEXT_MENU, kHandHotspot);

	bitmap = CreateCursorBitmap(gCursorSize,
		kCursorCopyBits, 7034, gCursorShadow / 10.0);
	_InitCursor(fCursorCopy, (uint8*)bitmap.Bits(),
		B_CURSOR_ID_COPY, kHandHotspot);

	bitmap = CreateCursorBitmap(gCursorSize,
		kCursorCreateLinkBits, 6909, gCursorShadow / 10.0);
	_InitCursor(fCursorCreateLink, (uint8*)bitmap.Bits(),
		B_CURSOR_ID_CREATE_LINK, kHandHotspot);

	bitmap = CreateCursorBitmap(gCursorSize,
		kCursorCrossHairBits, 4079, gCursorShadow / 10.0);
	_InitCursor(fCursorCrossHair, (uint8*)bitmap.Bits(),
		B_CURSOR_ID_CROSS_HAIR, ScaleHotspot(BPoint(10, 10)));

	bitmap = CreateCursorBitmap(gCursorSize,
		kCursorFollowLinkBits, 3974, gCursorShadow / 10.0);
	_InitCursor(fCursorFollowLink, (uint8*)bitmap.Bits(),
		B_CURSOR_ID_FOLLOW_LINK, ScaleHotspot(BPoint(5, 0)));

	bitmap = CreateCursorBitmap(gCursorSize,
		kCursorGrabBits, 3874, gCursorShadow / 10.0);
	_InitCursor(fCursorGrab, (uint8*)bitmap.Bits(),
		B_CURSOR_ID_GRAB, kHandHotspot);

	bitmap = CreateCursorBitmap(gCursorSize,
		kCursorGrabbingBits, 3732, gCursorShadow / 10.0);
	_InitCursor(fCursorGrabbing, (uint8*)bitmap.Bits(),
		B_CURSOR_ID_GRABBING, kHandHotspot);

	bitmap = CreateCursorBitmap(gCursorSize,
		kCursorHelpBits, 3725, gCursorShadow / 10.0);
	_InitCursor(fCursorHelp, (uint8*)bitmap.Bits(),
		B_CURSOR_ID_HELP, ScaleHotspot(BPoint(0, 8)));

	bitmap = CreateCursorBitmap(gCursorSize,
		kCursorIBeamBits, 3171, gCursorShadow / 10.0);
	_InitCursor(fCursorIBeam, (uint8*)bitmap.Bits(),
		B_CURSOR_ID_I_BEAM, ScaleHotspot(BPoint(7, 9)));

	bitmap = CreateCursorBitmap(gCursorSize,
		kCursorIBeamHorizontalBits, 3171, gCursorShadow / 10.0);
	_InitCursor(fCursorIBeamHorizontal, (uint8*)bitmap.Bits(),
		B_CURSOR_ID_I_BEAM_HORIZONTAL, ScaleHotspot(BPoint(8, 8)));

	bitmap = CreateCursorBitmap(gCursorSize,
		kCursorMoveBits, 3321, gCursorShadow / 10.0);
	_InitCursor(fCursorMove, (uint8*)bitmap.Bits(),
		B_CURSOR_ID_MOVE, kResizeHotspot);

	_InitCursor(fCursorNoCursor, 0, B_CURSOR_ID_NO_CURSOR, BPoint(0, 0));

	bitmap = CreateCursorBitmap(gCursorSize,
		kCursorNotAllowedBits, 3549, gCursorShadow / 10.0);
	_InitCursor(fCursorNotAllowed, (uint8*)bitmap.Bits(),
		B_CURSOR_ID_NOT_ALLOWED, ScaleHotspot(BPoint(8, 8)));

	bitmap = CreateCursorBitmap(gCursorSize,
		kCursorProgressBits, 5363, gCursorShadow / 10.0);
	_InitCursor(fCursorProgress, (uint8*)bitmap.Bits(),
		B_CURSOR_ID_PROGRESS, ScaleHotspot(BPoint(7, 10)));

	bitmap = CreateCursorBitmap(gCursorSize,
		kCursorResizeEastBits, 2896, gCursorShadow / 10.0);
	_InitCursor(fCursorResizeEast, (uint8*)bitmap.Bits(),
		B_CURSOR_ID_RESIZE_EAST, kResizeHotspot);

	bitmap = CreateCursorBitmap(gCursorSize,
		kCursorResizeEastWestBits, 2971, gCursorShadow / 10.0);
	_InitCursor(fCursorResizeEastWest, (uint8*)bitmap.Bits(),
		B_CURSOR_ID_RESIZE_EAST_WEST, kResizeHotspot);

	bitmap = CreateCursorBitmap(gCursorSize,
		kCursorResizeNorthBits, 2896, gCursorShadow / 10.0);
	_InitCursor(fCursorResizeNorth, (uint8*)bitmap.Bits(),
		B_CURSOR_ID_RESIZE_NORTH, kResizeHotspot);

	bitmap = CreateCursorBitmap(gCursorSize,
		kCursorResizeNorthEastBits, 2896, gCursorShadow / 10.0);
	_InitCursor(fCursorResizeNorthEast, (uint8*)bitmap.Bits(),
		B_CURSOR_ID_RESIZE_NORTH_EAST, kResizeHotspot);

	bitmap = CreateCursorBitmap(gCursorSize,
		kCursorResizeNorthEastSouthWestBits, 2971, gCursorShadow / 10.0);
	_InitCursor(fCursorResizeNorthEastSouthWest,
		(uint8*)bitmap.Bits(),
		B_CURSOR_ID_RESIZE_NORTH_EAST_SOUTH_WEST, kResizeHotspot);

	bitmap = CreateCursorBitmap(gCursorSize,
		kCursorResizeNorthSouthBits, 2971, gCursorShadow / 10.0);
	_InitCursor(fCursorResizeNorthSouth, (uint8*)bitmap.Bits(),
		B_CURSOR_ID_RESIZE_NORTH_SOUTH, kResizeHotspot);

	bitmap = CreateCursorBitmap(gCursorSize,
		kCursorResizeNorthWestBits, 2896, gCursorShadow / 10.0);
	_InitCursor(fCursorResizeNorthWest, (uint8*)bitmap.Bits(),
		B_CURSOR_ID_RESIZE_NORTH_WEST, kResizeHotspot);

	bitmap = CreateCursorBitmap(gCursorSize,
		kCursorResizeNorthWestSouthEastBits, 2971, gCursorShadow / 10.0);
	_InitCursor(fCursorResizeNorthWestSouthEast,
		(uint8*)bitmap.Bits(),
		B_CURSOR_ID_RESIZE_NORTH_WEST_SOUTH_EAST, kResizeHotspot);

	bitmap = CreateCursorBitmap(gCursorSize,
		kCursorResizeSouthBits, 2896, gCursorShadow / 10.0);
	_InitCursor(fCursorResizeSouth, (uint8*)bitmap.Bits(),
		B_CURSOR_ID_RESIZE_SOUTH, kResizeHotspot);

	bitmap = CreateCursorBitmap(gCursorSize,
		kCursorResizeSouthEastBits, 2896, gCursorShadow / 10.0);
	_InitCursor(fCursorResizeSouthEast, (uint8*)bitmap.Bits(),
		B_CURSOR_ID_RESIZE_SOUTH_EAST, kResizeHotspot);

	bitmap = CreateCursorBitmap(gCursorSize,
		kCursorResizeSouthWestBits, 2896, gCursorShadow / 10.0);
	_InitCursor(fCursorResizeSouthWest, (uint8*)bitmap.Bits(),
		B_CURSOR_ID_RESIZE_SOUTH_WEST, kResizeHotspot);

	bitmap = CreateCursorBitmap(gCursorSize,
		kCursorResizeWestBits, 2896, gCursorShadow / 10.0);
	_InitCursor(fCursorResizeWest, (uint8*)bitmap.Bits(),
		B_CURSOR_ID_RESIZE_WEST, kResizeHotspot);

	bitmap = CreateCursorBitmap(gCursorSize,
		kCursorZoomInBits, 7384, gCursorShadow / 10.0);
	_InitCursor(fCursorZoomIn, (uint8*)bitmap.Bits(),
		B_CURSOR_ID_ZOOM_IN, ScaleHotspot(BPoint(6, 6)));

	bitmap = CreateCursorBitmap(gCursorSize,
		kCursorZoomOutBits, 7184, gCursorShadow / 10.0);
	_InitCursor(fCursorZoomOut, (uint8*)bitmap.Bits(),
		B_CURSOR_ID_ZOOM_OUT, ScaleHotspot(BPoint(6, 6)));
}


ServerCursor*
CursorManager::CreateCursor(team_id clientTeam, const uint8* cursorData)
{
	if (!Lock())
		return NULL;

	ServerCursorReference cursor(_FindCursor(clientTeam, cursorData), false);

	if (!cursor) {
		cursor.SetTo(new (std::nothrow) ServerCursor(cursorData), true);
		if (cursor) {
			cursor->SetOwningTeam(clientTeam);
			if (AddCursor(cursor) < B_OK)
				cursor = NULL;
		}
	}

	Unlock();

	return cursor.Detach();
}


ServerCursor*
CursorManager::CreateCursor(team_id clientTeam, BRect r, color_space format,
	int32 flags, BPoint hotspot, int32 bytesPerRow)
{
	if (!Lock())
		return NULL;

	ServerCursor* cursor = new (std::nothrow) ServerCursor(r, format, flags,
		hotspot, bytesPerRow);
	if (cursor != NULL) {
		cursor->SetOwningTeam(clientTeam);
		if (AddCursor(cursor) < B_OK) {
			delete cursor;
			cursor = NULL;
		}
	}

	Unlock();

	return cursor;
}


/*!	\brief Registers a cursor with the manager.
	\param cursor ServerCursor object to register
	\return The token assigned to the cursor or B_ERROR if cursor is NULL
*/
int32
CursorManager::AddCursor(ServerCursor* cursor, int32 token)
{
	if (!cursor)
		return B_BAD_VALUE;
	if (!Lock())
		return B_ERROR;

	if (!fCursorList.AddItem(cursor)) {
		Unlock();
		return B_NO_MEMORY;
	}

	if (token == -1)
		token = fTokenSpace.NewToken(kCursorToken, cursor);
	else
		fTokenSpace.SetToken(token, kCursorToken, cursor);

	cursor->fToken = token;
	cursor->AttachedToManager(this);

	Unlock();

	return token;
}


/*!	\brief Removes a cursor if it's not referenced anymore.

	If this was the last reference to this cursor, it will be deleted.
	Only if the cursor is deleted, \c true is returned.
*/
bool
CursorManager::RemoveCursor(ServerCursor* cursor)
{
	if (!Lock())
		return false;

	// TODO: this doesn't work as it looks like, and it's not safe!
	if (cursor->CountReferences() > 0) {
		// cursor has been referenced again in the mean time
		Unlock();
		return false;
	}

	_RemoveCursor(cursor);

	Unlock();
	return true;
}


/*!	\brief Removes and deletes all of an application's cursors
	\param signature Signature to which the cursors belong
*/
void
CursorManager::DeleteCursors(team_id team)
{
	if (!Lock())
		return;

	for (int32 index = fCursorList.CountItems(); index-- > 0;) {
		ServerCursor* cursor = (ServerCursor*)fCursorList.ItemAtFast(index);
		if (cursor->OwningTeam() == team)
			cursor->ReleaseReference();
	}

	Unlock();
}


/*!	\brief Sets all the cursors from a specified CursorSet
	\param path Path to the cursor set

	All cursors in the set will be assigned. If the set does not specify a
	cursor for a particular cursor specifier, it will remain unchanged.
	This function will fail if passed a NULL path, an invalid path, or the
	path to a non-CursorSet file.
*/
void
CursorManager::SetCursorSet(const char* path)
{
	BAutolock locker (this);

	CursorSet cursorSet(NULL);

	if (!path || cursorSet.Load(path) != B_OK)
		return;

	_LoadCursor(fCursorSystemDefault, cursorSet, B_CURSOR_ID_SYSTEM_DEFAULT);
	_LoadCursor(fCursorContextMenu, cursorSet, B_CURSOR_ID_CONTEXT_MENU);
	_LoadCursor(fCursorCopy, cursorSet, B_CURSOR_ID_COPY);
	_LoadCursor(fCursorCreateLink, cursorSet, B_CURSOR_ID_CREATE_LINK);
	_LoadCursor(fCursorCrossHair, cursorSet, B_CURSOR_ID_CROSS_HAIR);
	_LoadCursor(fCursorFollowLink, cursorSet, B_CURSOR_ID_FOLLOW_LINK);
	_LoadCursor(fCursorGrab, cursorSet, B_CURSOR_ID_GRAB);
	_LoadCursor(fCursorGrabbing, cursorSet, B_CURSOR_ID_GRABBING);
	_LoadCursor(fCursorHelp, cursorSet, B_CURSOR_ID_HELP);
	_LoadCursor(fCursorIBeam, cursorSet, B_CURSOR_ID_I_BEAM);
	_LoadCursor(fCursorIBeamHorizontal, cursorSet,
		B_CURSOR_ID_I_BEAM_HORIZONTAL);
	_LoadCursor(fCursorMove, cursorSet, B_CURSOR_ID_MOVE);
	_LoadCursor(fCursorNotAllowed, cursorSet, B_CURSOR_ID_NOT_ALLOWED);
	_LoadCursor(fCursorProgress, cursorSet, B_CURSOR_ID_PROGRESS);
	_LoadCursor(fCursorResizeEast, cursorSet, B_CURSOR_ID_RESIZE_EAST);
	_LoadCursor(fCursorResizeEastWest, cursorSet,
		B_CURSOR_ID_RESIZE_EAST_WEST);
	_LoadCursor(fCursorResizeNorth, cursorSet, B_CURSOR_ID_RESIZE_NORTH);
	_LoadCursor(fCursorResizeNorthEast, cursorSet,
		B_CURSOR_ID_RESIZE_NORTH_EAST);
	_LoadCursor(fCursorResizeNorthEastSouthWest, cursorSet,
		B_CURSOR_ID_RESIZE_NORTH_EAST_SOUTH_WEST);
	_LoadCursor(fCursorResizeNorthSouth, cursorSet,
		B_CURSOR_ID_RESIZE_NORTH_SOUTH);
	_LoadCursor(fCursorResizeNorthWest, cursorSet,
		B_CURSOR_ID_RESIZE_NORTH_WEST);
	_LoadCursor(fCursorResizeNorthWestSouthEast, cursorSet,
		B_CURSOR_ID_RESIZE_NORTH_WEST_SOUTH_EAST);
	_LoadCursor(fCursorResizeSouth, cursorSet, B_CURSOR_ID_RESIZE_SOUTH);
	_LoadCursor(fCursorResizeSouthEast, cursorSet,
		B_CURSOR_ID_RESIZE_SOUTH_EAST);
	_LoadCursor(fCursorResizeSouthWest, cursorSet,
		B_CURSOR_ID_RESIZE_SOUTH_WEST);
	_LoadCursor(fCursorResizeWest, cursorSet, B_CURSOR_ID_RESIZE_WEST);
	_LoadCursor(fCursorZoomIn, cursorSet, B_CURSOR_ID_ZOOM_IN);
	_LoadCursor(fCursorZoomOut, cursorSet, B_CURSOR_ID_ZOOM_OUT);
}


/*!	\brief Acquire the cursor which is used for a particular system cursor
	\param which Which system cursor to get
	\return Pointer to the particular cursor used or NULL if which is
	invalid or the cursor has not been assigned
*/
ServerCursor*
CursorManager::GetCursor(BCursorID which)
{
	BAutolock locker(this);

	switch (which) {
		case B_CURSOR_ID_SYSTEM_DEFAULT:
			return fCursorSystemDefault;
		case B_CURSOR_ID_CONTEXT_MENU:
			return fCursorContextMenu;
		case B_CURSOR_ID_COPY:
			return fCursorCopy;
		case B_CURSOR_ID_CREATE_LINK:
			return fCursorCreateLink;
		case B_CURSOR_ID_CROSS_HAIR:
			return fCursorCrossHair;
		case B_CURSOR_ID_FOLLOW_LINK:
			return fCursorFollowLink;
		case B_CURSOR_ID_GRAB:
			return fCursorGrab;
		case B_CURSOR_ID_GRABBING:
			return fCursorGrabbing;
		case B_CURSOR_ID_HELP:
			return fCursorHelp;
		case B_CURSOR_ID_I_BEAM:
			return fCursorIBeam;
		case B_CURSOR_ID_I_BEAM_HORIZONTAL:
			return fCursorIBeamHorizontal;
		case B_CURSOR_ID_MOVE:
			return fCursorMove;
		case B_CURSOR_ID_NO_CURSOR:
			return fCursorNoCursor;
		case B_CURSOR_ID_NOT_ALLOWED:
			return fCursorNotAllowed;
		case B_CURSOR_ID_PROGRESS:
			return fCursorProgress;
		case B_CURSOR_ID_RESIZE_EAST:
			return fCursorResizeEast;
		case B_CURSOR_ID_RESIZE_EAST_WEST:
			return fCursorResizeEastWest;
		case B_CURSOR_ID_RESIZE_NORTH:
			return fCursorResizeNorth;
		case B_CURSOR_ID_RESIZE_NORTH_EAST:
			return fCursorResizeNorthEast;
		case B_CURSOR_ID_RESIZE_NORTH_EAST_SOUTH_WEST:
			return fCursorResizeNorthEastSouthWest;
		case B_CURSOR_ID_RESIZE_NORTH_SOUTH:
			return fCursorResizeNorthSouth;
		case B_CURSOR_ID_RESIZE_NORTH_WEST:
			return fCursorResizeNorthWest;
		case B_CURSOR_ID_RESIZE_NORTH_WEST_SOUTH_EAST:
			return fCursorResizeNorthWestSouthEast;
		case B_CURSOR_ID_RESIZE_SOUTH:
			return fCursorResizeSouth;
		case B_CURSOR_ID_RESIZE_SOUTH_EAST:
			return fCursorResizeSouthEast;
		case B_CURSOR_ID_RESIZE_SOUTH_WEST:
			return fCursorResizeSouthWest;
		case B_CURSOR_ID_RESIZE_WEST:
			return fCursorResizeWest;
		case B_CURSOR_ID_ZOOM_IN:
			return fCursorZoomIn;
		case B_CURSOR_ID_ZOOM_OUT:
			return fCursorZoomOut;

		default:
			return NULL;
	}
}


/*!	\brief Internal function which finds the cursor with a particular ID
	\param token ID of the cursor to find
	\return The cursor or NULL if not found
*/
ServerCursor*
CursorManager::FindCursor(int32 token)
{
	if (!Lock())
		return NULL;

	ServerCursor* cursor;
	if (fTokenSpace.GetToken(token, kCursorToken, (void**)&cursor) != B_OK)
		cursor = NULL;

	Unlock();

	return cursor;
}


/*!	\brief Initializes a predefined system cursor.

	This method must only be called in the CursorManager's constructor,
	as it may throw exceptions.
*/
void
CursorManager::_InitCursor(ServerCursor*& cursorMember,
	const uint8* cursorBits, BCursorID id, const BPoint& hotSpot)
{
	if (cursorBits) {
		cursorMember = new ServerCursor(cursorBits, gCursorSize,
			gCursorSize, kCursorFormat);
	} else
		cursorMember = new ServerCursor(kCursorNoCursor, 1, 1, kCursorFormat);

	cursorMember->SetHotSpot(hotSpot);
	AddCursor(cursorMember, id);
}


void
CursorManager::_LoadCursor(ServerCursor*& cursorMember, const CursorSet& set,
	BCursorID id)
{
	ServerCursor* cursor;
	if (set.FindCursor(id, &cursor) == B_OK) {
		int32 index = fCursorList.IndexOf(cursorMember);
		if (index >= 0) {
			ServerCursor** items = reinterpret_cast<ServerCursor**>(
				fCursorList.Items());
			items[index] = cursor;
		}
		delete cursorMember;
		cursorMember = cursor;
	}
}


ServerCursor*
CursorManager::_FindCursor(team_id clientTeam, const uint8* cursorData)
{
	int32 count = fCursorList.CountItems();
	for (int32 i = 0; i < count; i++) {
		ServerCursor* cursor = (ServerCursor*)fCursorList.ItemAtFast(i);
		if (cursor->OwningTeam() == clientTeam
			&& cursor->CursorData()
			&& memcmp(cursor->CursorData(), cursorData, 68) == 0) {
			return cursor;
		}
	}
	return NULL;
}


void
CursorManager::_RemoveCursor(ServerCursor* cursor)
{
	fCursorList.RemoveItem(cursor);
	fTokenSpace.RemoveToken(cursor->fToken);
}
