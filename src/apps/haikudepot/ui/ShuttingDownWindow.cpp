/*
 * Copyright 2021, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "ShuttingDownWindow.h"

#include <Catalog.h>
#include <LayoutBuilder.h>
#include <Locker.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ShuttingDownWindow"

#define WINDOW_FRAME BRect(0, 0, 240, 120)


ShuttingDownWindow::ShuttingDownWindow(BWindow* parent)
	:
	BWindow(WINDOW_FRAME, B_TRANSLATE("Cleaning up" B_UTF8_ELLIPSIS),
		B_FLOATING_WINDOW_LOOK, B_MODAL_SUBSET_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS
			| B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_CLOSABLE )
{
	AddToSubset(parent);

	// we have to set initial text color to panel text color
	const char* name = "shutting down message";
	rgb_color textColor = ui_color(B_PANEL_TEXT_COLOR);
	BTextView* textView = new BTextView(name, be_plain_font, &textColor, B_WILL_DRAW);
	textView->AdoptSystemColors();
	textView->MakeEditable(false);
	textView->MakeSelectable(false);
	textView->SetText(B_TRANSLATE("HaikuDepot is stopping or completing "
		"running operations before quitting."));

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
			.SetInsets(B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING,
				B_USE_WINDOW_SPACING, B_USE_DEFAULT_SPACING)
			.Add(textView)
		.End();

	CenterOnScreen();
	ResizeToPreferred();
}


ShuttingDownWindow::~ShuttingDownWindow()
{
}

