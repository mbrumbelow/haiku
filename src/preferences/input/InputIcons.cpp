/*
 * Copyright 2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "InputIcons.h"

#include <Application.h>
#include <File.h>
#include <IconUtils.h>
#include <Resources.h>
#include <Roster.h>

#include "IconHandles.h"

#define ICON_SIZE 32

const BRect InputIcons::sBounds(0, 0, 15, 15);


InputIcons::IconSet::IconSet()
{
}



InputIcons::InputIcons()
	:
	mouseIcon(sBounds, B_CMAP8),
	touchpadIcon(sBounds, B_CMAP8),
	keyboardIcon(sBounds, B_CMAP8)
{
	app_info info;
	be_app->GetAppInfo(&info);
	BFile executableFile(&info.ref, B_READ_ONLY);
	BResources resources(&executableFile);
	resources.PreloadResourceType(B_COLOR_8_BIT_TYPE);

	_LoadBitmap(&resources);
}


void
InputIcons::_LoadBitmap(BResources* resources)
{
	const uint8* mouse;
	const uint8* touchpad;
	const uint8* keyboard;

	size_t size;

	mouse = (const uint8*)resources->LoadResource(
		B_VECTOR_ICON_TYPE, "mouse_icon", &size);
	if (mouse) {
		mouseIcon = new BBitmap(BRect(0, 0, ICON_SIZE, ICON_SIZE),
			0, B_RGBA32);
		BIconUtils::GetVectorIcon(mouse, size, &mouseIcon);
	}

	touchpad = (const uint8*)resources->LoadResource(
		B_VECTOR_ICON_TYPE, "touchpad_icon", &size);
	if (touchpad) {
		touchpadIcon = new BBitmap(BRect(0, 0, ICON_SIZE, ICON_SIZE),
			0, B_RGBA32);
		BIconUtils::GetVectorIcon(touchpad, size, &touchpadIcon);
	}

	keyboard = (const uint8*)resources->LoadResource(
		B_VECTOR_ICON_TYPE, "keyboard_icon", &size);
	if (keyboard) {
		keyboardIcon = new BBitmap(BRect(0, 0, ICON_SIZE, ICON_SIZE),
			0, B_RGBA32);
		BIconUtils::GetVectorIcon(keyboard, size, &keyboardIcon);
	}

}


BRect
InputIcons::IconRectAt(const BPoint& topLeft)
{
	return BRect(sBounds).OffsetToSelf(topLeft);
}
