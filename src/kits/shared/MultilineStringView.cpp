/*
 * Copyright 2018 Kacper Kasper, kacperkasper@gmail.com
 * Distributed under the terms of the MIT license.
 */
#include "MultilineStringView.h"

#include <GroupLayout.h>
#include <String.h>
#include <StringList.h>
#include <StringView.h>
#include <View.h>
#include <Window.h>


namespace BPrivate {

BMultilineStringView::BMultilineStringView(const char* name, const char* string)
	:
	BView(name, B_AUTO_UPDATE_SIZE_LIMITS)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	SetLayout(new BGroupLayout(B_VERTICAL, 0));

	BStringList lines;
	BString(string).Split("\n", false, lines);
	for (int i = 0; i < lines.CountStrings(); i++) {
		AddChild(new BStringView("", lines.StringAt(i)));
	}
}

} // namespace BPrivate
