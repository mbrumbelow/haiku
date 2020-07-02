/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#include "WacomSettingsView.h"

#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Debug.h>
#include <InterfaceDefs.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <SeparatorView.h>
#include <Slider.h>
#include <TabView.h>
#include <TextControl.h>
#include <TranslationUtils.h>
#include <Window.h>

#include "InputConstants.h"
#include "WacomPenView.h"
#include "WacomMappingView.h"


//	#pragma mark -

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "WacomSettingsView"

WacomSettingsView::WacomSettingsView()
	:
	BGroupView()
{

	BTabView* tabView = new BTabView("tabview", B_WIDTH_FROM_LABEL);

	fWacomPenView = new WacomPenView();
	fWacomMappingView = new WacomMappingView();

	tabView->AddTab(fWacomPenView);
	tabView->AddTab(fWacomMappingView);
	tabView->SetBorder(B_NO_BORDER);

	fDefaultButton = new BButton(B_TRANSLATE("Defaults"),
		new BMessage(kMsgDefaults));

	fRevertButton = new BButton(B_TRANSLATE("Revert"),
		new BMessage(kMsgRevert));
	fRevertButton->SetEnabled(false);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.Add(tabView)
			.Add(new BSeparatorView(B_HORIZONTAL))
				.AddGroup(B_HORIZONTAL)
				.Add(fDefaultButton)
				.Add(fRevertButton)
				.AddGlue()
				.End()
		.End();
}


WacomSettingsView::~WacomSettingsView()
{

}


void
WacomSettingsView::AttachedToWindow()
{
//	UpdateFromSettings();
}
