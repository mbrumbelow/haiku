/*
 * Copyright 2001-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef APP_FONT_MANAGER_H
#define APP_FONT_MANAGER_H


#include <AutoDeleter.h>
#include <HashMap.h>
#include <Looper.h>
#include <ObjectList.h>
#include <Referenceable.h>

#include "FontManager.h"

#include <ft2build.h>
#include FT_FREETYPE_H

class BEntry;
class BPath;
struct node_ref;

class FontFamily;
class FontStyle;
class ServerFont;


/*!
	\class AppFontManager AppFontManager.h
	\brief Manager for the largest part of the font subsystem
*/
class AppFontManager : public BLooper {
public:
								AppFontManager();
	virtual						~AppFontManager();

	virtual	void				MessageReceived(BMessage* message);

			int32				CountFamilies();

			int32				CountStyles(const char* family);
			int32				CountStyles(uint16 familyID);
			FontFamily*			FamilyAt(int32 index) const;

			FontFamily*			GetFamily(uint16 familyID) const;
			FontFamily*			GetFamily(const char* name);

			FontStyle*			GetStyleByIndex(const char* family,
									int32 index);
			FontStyle*			GetStyleByIndex(uint16 familyID, int32 index);
			FontStyle*			GetStyle(const char* family, const char* style,
									uint16 familyID = 0xffff,
									uint16 styleID = 0xffff, uint16 face = 0);
			FontStyle*			GetStyle(const char *family, uint16 styleID);
			FontStyle*			GetStyle(uint16 familyID, uint16 styleID) const;
			FontStyle*			FindStyleMatchingFace(uint16 face) const;

			void				RemoveStyle(FontStyle* style);
				// This call must not be used by anything else than class
				// FontStyle.

			status_t			AddUserFontFromFile(const char* path,
									uint16& familyID, uint16& styleID);
			status_t			AddUserFontFromMemory(const FT_Byte* fontAddress,
									uint32 size, uint16& familyID, uint16& styleID);
			status_t			RemoveUserFont(uint16 familyID, uint16 styleID);

private:
			FontFamily*			_FindFamily(const char* family) const;

			status_t			_AddUserFont(FT_Face face, node_ref nodeRef,
									const char* path,
									uint16& familyID, uint16& styleID);

private:
			struct FontKey {
				FontKey(uint16 family, uint16 style)
					: familyID(family), styleID(style) {}

				uint32 GetHashCode() const
				{
					return familyID | (styleID << 16UL);
				}

				bool operator==(const FontKey& other) const
				{
					return familyID == other.familyID
						&& styleID == other.styleID;
				}

				uint16 		familyID, styleID;
			};


private:
			typedef BObjectList<FontFamily>			FamilyList;

			FamilyList			fFamilies;

			HashMap<FontKey, BReference<FontStyle> > fStyleHashTable;

			int32				fNextID;
};


#endif	/* APP_FONT_MANAGER_H */
