/*
 * Copyright 2001-2016, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */


/*!	Manages user font families and styles */


#include "AppFontManager.h"

#include <new>
#include <stdint.h>
#include <syslog.h>

#include <Autolock.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <String.h>

#include "FontFamily.h"
#include "FontManager.h"
#include "ServerConfig.h"
#include "ServerFont.h"


#define TRACE_FONT_MANAGER
#ifdef TRACE_FONT_MANAGER
#	define FTRACE(x) debug_printf x
#else
#	define FTRACE(x) ;
#endif


static int
compare_font_families(const FontFamily* a, const FontFamily* b)
{
	int result = strcmp(a->Name(), b->Name());

	return result;
}


//	#pragma mark -


//! Does basic set up so that directories can be scanned
AppFontManager::AppFontManager()
	: BLooper("AppFontManager"),
	fFamilies(20)
{
	srand(time(NULL));
	fNextID = (INT32_MAX / 2) + (rand() % 10000);
}


//! Frees items allocated in the constructor and shuts down FreeType
AppFontManager::~AppFontManager()
{
	for (int32 i = fFamilies.CountItems(); i-- > 0;) {
		FontFamily* family = fFamilies.ItemAt(i);
		for (int32 j = 0; j < family->CountStyles(); ++j) {
			uint16 familyID = family->ID();
			uint16 styleID = family->StyleAt(j)->ID();
			FontKey fKey(familyID, styleID);
			FontStyle *styleRef = fStyleHashTable.Get(fKey);
			fStyleHashTable.Remove(fKey);
			styleRef->ReleaseReference();
		}

		delete fFamilies.ItemAt(i);
	}
}


void
AppFontManager::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BLooper::MessageReceived(message);
			break;
	}
}


/*!	\brief Counts the number of font families available
	\return The number of unique font families currently available
*/
int32
AppFontManager::CountFamilies()
{
	return fFamilies.CountItems();
}


/*!	\brief Counts the number of styles available in a font family
	\param family Name of the font family to scan
	\return The number of font styles currently available for the font family
*/
int32
AppFontManager::CountStyles(const char *familyName)
{
	FontFamily *family = GetFamily(familyName);
	if (family)
		return family->CountStyles();

	return 0;
}


/*!	\brief Counts the number of styles available in a font family
	\param family Name of the font family to scan
	\return The number of font styles currently available for the font family
*/
int32
AppFontManager::CountStyles(uint16 familyID)
{
	FontFamily *family = GetFamily(familyID);
	if (family)
		return family->CountStyles();

	return 0;
}


FontFamily*
AppFontManager::FamilyAt(int32 index) const
{
	return fFamilies.ItemAt(index);
}


FontFamily*
AppFontManager::_FindFamily(const char* name) const
{
	if (name == NULL)
		return NULL;

	FontFamily family(name, 0);

	return const_cast<FontFamily*>(fFamilies.BinarySearch(family,
		compare_font_families));
}


/*!	\brief Locates a FontFamily object by name
	\param name The family to find
	\return Pointer to the specified family or NULL if not found.
*/
FontFamily*
AppFontManager::GetFamily(const char* name)
{
	if (name == NULL)
		return NULL;

	FontFamily* family = _FindFamily(name);
	if (family != NULL)
		return family;

	return _FindFamily(name);
}


FontFamily*
AppFontManager::GetFamily(uint16 familyID) const
{
	FontKey key(familyID, 0);
	FontStyle* style = fStyleHashTable.Get(key);
	if (style != NULL)
		return style->Family();

	return NULL;
}


FontStyle*
AppFontManager::GetStyleByIndex(const char* familyName, int32 index)
{
	FontFamily* family = GetFamily(familyName);
	if (family != NULL)
		return family->StyleAt(index);

	return NULL;
}


FontStyle*
AppFontManager::GetStyleByIndex(uint16 familyID, int32 index)
{
	FontFamily* family = GetFamily(familyID);
	if (family != NULL)
		return family->StyleAt(index);

	return NULL;
}


/*!	\brief Retrieves the FontStyle object that comes closest to the one
		specified.

	\param family The font's family or NULL in which case \a familyID is used
	\param style The font's style or NULL in which case \a styleID is used
	\param familyID will only be used if \a family is NULL (or empty)
	\param styleID will only be used if \a style is NULL (or empty)
	\param face is used to specify the style if both \a style is NULL or empty
		and styleID is 0xffff.

	\return The FontStyle having those attributes or NULL if not available
*/
FontStyle*
AppFontManager::GetStyle(const char* familyName, const char* styleName,
	uint16 familyID, uint16 styleID, uint16 face)
{
	FontFamily* family;

	// find family

	if (familyName != NULL && familyName[0])
		family = GetFamily(familyName);
	else
		family = GetFamily(familyID);

	if (family == NULL)
		return NULL;

	// find style

	if (styleName != NULL && styleName[0]) {
		FontStyle* fontStyle = family->GetStyle(styleName);
		if (fontStyle != NULL)
			return fontStyle;

		return family->GetStyle(styleName);
	}

	if (styleID != 0xffff)
		return family->GetStyleByID(styleID);

	// try to get from face
	return family->GetStyleMatchingFace(face);
}


/*!	\brief Retrieves the FontStyle object
	\param family ID for the font's family
	\param style ID of the font's style
	\return The FontStyle having those attributes or NULL if not available
*/
FontStyle*
AppFontManager::GetStyle(uint16 familyID, uint16 styleID) const
{
	FontKey key(familyID, styleID);
	return fStyleHashTable.Get(key);
}


/*!	\brief If you don't find your preferred font style, but are anxious
		to have one fitting your needs, you may want to use this method.
*/
FontStyle*
AppFontManager::FindStyleMatchingFace(uint16 face) const
{
	int32 count = fFamilies.CountItems();

	for (int32 i = 0; i < count; i++) {
		FontFamily* family = fFamilies.ItemAt(i);
		FontStyle* style = family->GetStyleMatchingFace(face);
		if (style != NULL)
			return style;
	}

	return NULL;
}


/*!	\brief This call is used by the FontStyle class - and the FontStyle class
		only - to remove itself from the font manager.
	At this point, the style is already no longer available to the user.
*/
void
AppFontManager::RemoveStyle(FontStyle* style)
{
	if (style == NULL)
		return;

	FTRACE(("FontManager::RemoveStyle style removing: %s\n",
		style->Name()));

	FontFamily* family = style->Family();
	if (family != NULL) {
		if (family->RemoveStyle(style)
			&& family->CountStyles() == 0) {
			fFamilies.RemoveItem(family);
		}
	}
}


status_t
AppFontManager::_AddUserFont(FT_Face face, node_ref nodeRef, const char* path,
	uint16& familyID, uint16& styleID)
{
	FontFamily* family = _FindFamily(face->family_name);
	if (family != NULL &&
		family->HasStyle(face->style_name)) {
		// prevent adding the same style twice
		// (this indicates a problem with the installed fonts maybe?)
		FT_Done_Face(face);
		return B_NAME_IN_USE;
	}

	if (family == NULL) {
		family = new (std::nothrow) FontFamily(face->family_name, fNextID++);

		if (family == NULL
			|| !fFamilies.BinaryInsert(family, compare_font_families)) {
			delete family;
			FT_Done_Face(face);
			return B_NO_MEMORY;
		}
	}

	FTRACE(("\tadd style: %s, %s\n", face->family_name, face->style_name));

	// the FontStyle takes over ownership of the FT_Face object
	FontStyle* style = new (std::nothrow) FontStyle(nodeRef, path, face);

	if (style == NULL || !family->AddStyle(style, this)) {
		delete style;
		delete family;
		return B_NO_MEMORY;
	}

	familyID = style->Family()->ID();
	styleID = style->ID();

	fStyleHashTable.Put(FontKey(familyID, styleID), style);

	return B_OK;
}


/*!	\brief Adds the FontFamily/FontStyle that is represented by this path.
*/
status_t
AppFontManager::AddUserFontFromFile(const char* path,
	uint16& familyID, uint16& styleID)
{
	BEntry entry;
	status_t status = entry.SetTo(path);
	if (status != B_OK)
		return status;

	node_ref nodeRef;
	status = entry.GetNodeRef(&nodeRef);
	if (status < B_OK)
		return status;

	FT_Face face;
	FT_Error error = FT_New_Face(gFreeTypeLibrary, path, 0, &face);
	if (error != 0)
		return error;

	status = _AddUserFont(face, nodeRef, path, familyID, styleID);

	return status;
}


/*!	\brief Adds the FontFamily/FontStyle that is represented by the area in memory.
*/
status_t
AppFontManager::AddUserFontFromMemory(const FT_Byte* fontAddress, uint32 size,
	uint16& familyID, uint16& styleID)
{
	node_ref nodeRef;
	status_t status;

	FT_Face face;
	FT_Error error = FT_New_Memory_Face(gFreeTypeLibrary, fontAddress, size, 0,
		&face);
	if (error != 0)
		return error;

	status = _AddUserFont(face, nodeRef, "", familyID, styleID);

	return status;
}


/*!	\brief Removes the FontFamily/FontStyle from the font manager.
*/
status_t
AppFontManager::RemoveUserFont(uint16 familyID, uint16 styleID)
{
	FontKey fKey(familyID, styleID);
	FontStyle *styleRef = fStyleHashTable.Get(fKey);
	fStyleHashTable.Remove(fKey);

	FontFamily* family = styleRef->Family();
	bool removed = family->RemoveStyle(styleRef, this);
	
	if (!removed)
		syslog(LOG_DEBUG, "FontManager::RemoveUserFont style not removed from family\n");

	fFamilies.RemoveItem(family);
	delete family;

	styleRef->ReleaseReference();

	return B_OK;
}