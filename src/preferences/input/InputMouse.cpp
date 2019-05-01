/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#include "InputMouse.h"

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
#include <TextControl.h>
#include <TranslationUtils.h>
#include <Window.h>

#include "InputConstants.h"
#include "MouseSettings.h"
#include "MouseView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SettingsView"

SettingsView::SettingsView(MouseSettings& settings)
	: BControl("Mouse", NULL, NULL, B_WILL_DRAW | B_NAVIGABLE),
	fSettings(settings)
{
	SetLayout(new BGroupLayout(B_VERTICAL));
	BBox* scrollBox = new BBox("Mouse");
	scrollBox->SetLabel(B_TRANSLATE("Mouse View"));

	fTypeMenu = new BPopUpMenu("Buttons");
	fTypeMenu->AddItem(new BMenuItem(B_TRANSLATE("1-Button"),
		new BMessage(kMsgMouseType)));
	fTypeMenu->AddItem(new BMenuItem(B_TRANSLATE("2-Button"),
		new BMessage(kMsgMouseType)));
	fTypeMenu->AddItem(new BMenuItem(B_TRANSLATE("3-Button"),
		new BMessage(kMsgMouseType)));

	BMenuField* typeField = new BMenuField(B_TRANSLATE("Mouse type:"),
		fTypeMenu);
	typeField->SetAlignment(B_ALIGN_LEFT);

	fMouseView = new MouseView(fSettings);

	float spacing = be_control_look->DefaultItemSpacing();

	const char* label = B_TRANSLATE("Double-click test area");
	BTextControl* doubleClickTextControl = new BTextControl(NULL,
		label, NULL);
	doubleClickTextControl->SetAlignment(B_ALIGN_LEFT, B_ALIGN_CENTER);
	doubleClickTextControl->SetExplicitMinSize(
		BSize(StringWidth(label), B_SIZE_UNSET));

	BView* scrollPrefLeftLayout = BLayoutBuilder::Group<>(B_VERTICAL, 0)
		.Add(fMouseView)
		.Add(doubleClickTextControl)
		.AddStrut(spacing)
		.AddGroup(B_HORIZONTAL, 0)
			.AddStrut(spacing * 2)
			.End()
		.AddGlue()
		.View();

	BGroupLayout* scrollPrefLayout = new BGroupLayout(B_HORIZONTAL);
	scrollPrefLayout->SetSpacing(spacing);
	scrollPrefLayout->SetInsets(spacing,
		scrollBox->TopBorderOffset() * 2 + spacing, spacing, spacing);
	scrollBox->SetLayout(scrollPrefLayout);

	scrollPrefLayout->AddView(scrollPrefLeftLayout);
	scrollPrefLayout->AddItem(BSpaceLayoutItem::CreateVerticalStrut(spacing
		* 1.5));

	BLayoutBuilder::Group<>(this, B_HORIZONTAL, B_USE_DEFAULT_SPACING)
		.Add(scrollBox)
		.Add(typeField)
	.End();
}


SettingsView::~SettingsView()
{
}


void
SettingsView::AttachedToWindow()
{
	UpdateFromSettings();
}


void
SettingsView::SetMouseType(int32 type)
{
	fMouseView->UpdateFromSettings();
}


void
SettingsView::MouseMapUpdated()
{
	fMouseView->MouseMapUpdated();
}


void
SettingsView::UpdateFromSettings()
{
	BMenuItem* item = fTypeMenu->ItemAt(fSettings.MouseType() - 1);
	if (item != NULL)
		item->SetMarked(true);

	fMouseView->SetMouseType(fSettings.MouseType());
}