/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PadView.h"

#include <stdio.h>

#include <Directory.h>
#include <File.h>
#include <Application.h>
#include <Catalog.h>
#include <GroupLayout.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <Region.h>
#include <Screen.h>
#include <SpaceLayoutItem.h>

#include "LaunchButton.h"
#include "MainWindow.h"
#include "App.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "LaunchBox"


static bigtime_t sActivationDelay = 40000;


enum {
	MSG_TOGGLE_LAYOUT			= 'tgll',
	MSG_SET_ICON_SIZE			= 'stis',
	MSG_SET_IGNORE_DOUBLECLICK	= 'strd'
};


PadView::PadView(const char* name)
	: BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE, NULL),
	  fDragging(false),
	  fClickTime(0),
	  fButtonLayout(new BGroupLayout(B_HORIZONTAL, 4)),
	  fIconSize(32)
{
//	SetViewColor(B_TRANSPARENT_32_BIT);
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	SetLowUIColor(B_PANEL_BACKGROUND_COLOR);
	get_click_speed(&sActivationDelay);

	BScreen screen(Window());
	float screenHeight = screen.Frame().Height();
	if (screenHeight >= 2160) // -> 4K displays and above (typically fairly high DPI too)
		fDefaultIconSize = 64;
	else if (screenHeight > 1080) // -> Bigger than the "modern standard" 1920x1080
		fDefaultIconSize = 48;
	else
		fDefaultIconSize = 32;

	fIconSize = fDefaultIconSize;

	fButtonLayout->SetInsets(7, 2, 2, 2);
	SetLayout(fButtonLayout);
}


PadView::~PadView()
{
}


void
PadView::Draw(BRect updateRect)
{
	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color borderLight = tint_color(background, B_DARKEN_1_TINT);
	rgb_color borderShadow = tint_color(background, B_DARKEN_2_TINT);
	rgb_color grabberLight = mix_color(background, ui_color(B_PANEL_TEXT_COLOR), 50);
	rgb_color grabberShadow = mix_color(background, ui_color(B_PANEL_TEXT_COLOR), 25);

	BRect r(Bounds());

	// Draw window outline...
	BeginLineArray(4);
		AddLine(BPoint(r.left, r.bottom), BPoint(r.left, r.top), borderLight);
		AddLine(BPoint(r.left + 1.0, r.top), BPoint(r.right, r.top), borderLight);
		AddLine(BPoint(r.right, r.top + 1.0), BPoint(r.right, r.bottom), borderShadow);
		AddLine(BPoint(r.right - 1.0, r.bottom), BPoint(r.left + 1.0, r.bottom), borderShadow);
	EndLineArray();
	r.InsetBy(1.0, 1.0);
	StrokeRect(r, B_SOLID_LOW);
	r.InsetBy(1.0, 1.0);

	// Draw grabber... (the dotted window grip along the
	// left (when horizontal) or top (when vertical) side)
	BPoint dot = r.LeftTop();
	int32 current;
	int32 stop;
	BPoint offset;
	BPoint next;
	if (Orientation() == B_VERTICAL) {
		current = (int32)dot.x;
		stop = (int32)r.right;
		offset = BPoint(0, 1);
		next = BPoint(1, -4);
		r.top += 5.0;
	} else {
		current = (int32)dot.y;
		stop = (int32)r.bottom;
		offset = BPoint(1, 0);
		next = BPoint(-4, 1);
		r.left += 5.0;
	}

	int32 num = 1;
	while (current <= stop) {
		rgb_color col1;
		rgb_color col2;
		if (num == 1) {
			col1 = grabberShadow;
			col2 = background;
		} else if (num == 2) {
			col1 = background;
			col2 = grabberLight;
		} else {
			col1 = background;
			col2 = background;
			num = 0;
		}

		SetHighColor(col1);
		StrokeLine(dot, dot, B_SOLID_HIGH);
		SetHighColor(col2);
		dot += offset;
		StrokeLine(dot, dot, B_SOLID_HIGH);
		dot += offset;
		StrokeLine(dot, dot, B_SOLID_LOW);
		dot += offset;
		SetHighColor(col1);
		StrokeLine(dot, dot, B_SOLID_HIGH);
		dot += offset;
		SetHighColor(col2);
		StrokeLine(dot, dot, B_SOLID_HIGH);

		// next pixel
		num++;
		dot += next;
		current++;
	}

	FillRect(r, B_SOLID_LOW);
}


void
PadView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_TOGGLE_LAYOUT:
			if (fButtonLayout->Orientation() == B_HORIZONTAL) {
				fButtonLayout->SetInsets(2, 7, 2, 2);
				fButtonLayout->SetOrientation(B_VERTICAL);
			} else {
				fButtonLayout->SetInsets(7, 2, 2, 2);
				fButtonLayout->SetOrientation(B_HORIZONTAL);
			}
			break;

		case MSG_SET_ICON_SIZE:
			uint32 size;
			if (message->FindInt32("size", (int32*)&size) == B_OK)
				SetIconSize(size);
			break;

		case MSG_SET_IGNORE_DOUBLECLICK:
			SetIgnoreDoubleClick(!IgnoreDoubleClick());
			break;

		case B_COLORS_UPDATED:
		{
			Draw(Bounds());
			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
PadView::MouseDown(BPoint where)
{
	BWindow* window = Window();
	if (window == NULL)
		return;

	BRegion region;
	GetClippingRegion(&region);
	if (!region.Contains(where))
		return;

	bool handle = true;
	for (int32 i = 0; BView* child = ChildAt(i); i++) {
		if (child->Frame().Contains(where)) {
			handle = false;
			break;
		}
	}
	if (!handle)
		return;

	BMessage* message = window->CurrentMessage();
	if (message == NULL)
		return;

	uint32 buttons;
	message->FindInt32("buttons", (int32*)&buttons);
	if (buttons & B_SECONDARY_MOUSE_BUTTON) {
		BRect r = Bounds();
		r.InsetBy(2.0, 2.0);
		r.top += 6.0;
		if (r.Contains(where)) {
			DisplayMenu(where);
		} else {
			// sends the window to the back
			window->Activate(false);
		}
	} else {
		if (system_time() - fClickTime < sActivationDelay) {
			window->Minimize(true);
			fClickTime = 0;
		} else {
			window->Activate();
			fDragOffset = ConvertToScreen(where) - window->Frame().LeftTop();
			fDragging = true;
			SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
			fClickTime = system_time();
		}
	}
}


void
PadView::MouseUp(BPoint where)
{
	if (BWindow* window = Window()) {
		uint32 buttons;
		window->CurrentMessage()->FindInt32("buttons", (int32*)&buttons);
		if (buttons & B_PRIMARY_MOUSE_BUTTON
			&& system_time() - fClickTime < sActivationDelay
			&& window->IsActive())
			window->Activate();
	}
	fDragging = false;
}


void
PadView::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	MainWindow* window = dynamic_cast<MainWindow*>(Window());
	if (window == NULL)
		return;

	if (fDragging) {
		window->MoveTo(ConvertToScreen(where) - fDragOffset);
	} else if (window->AutoRaise()) {
		where = ConvertToScreen(where);
		BScreen screen(window);
		BRect frame = screen.Frame();
		BRect windowFrame = window->Frame();

		if (where.x == frame.left || where.x == frame.right
			|| where.y == frame.top || where.y == frame.bottom) {
			BPoint position = window->ScreenPosition();
			bool raise = false;

			if (fabs(0.5 - position.x) > fabs(0.5 - position.y)) {
				// left or right border
				if (where.y >= windowFrame.top
					&& where.y <= windowFrame.bottom) {
					if (position.x < 0.5 && where.x == frame.left)
						raise = true;
					else if (position.x > 0.5 && where.x == frame.right)
						raise = true;
				}
			} else {
				// top or bottom border
				if (where.x >= windowFrame.left && where.x <= windowFrame.right) {
					if (position.y < 0.5 && where.y == frame.top)
						raise = true;
					else if (position.y > 0.5 && where.y == frame.bottom)
						raise = true;
				}
			}

			if (raise)
				window->Activate();
		}
	}
}


void
PadView::AddButton(LaunchButton* button, LaunchButton* beforeButton)
{
	button->SetIconSize(fIconSize);

	if (beforeButton)
		fButtonLayout->AddView(fButtonLayout->IndexOfView(beforeButton), button);
	else
		fButtonLayout->AddView(button);

	_NotifySettingsChanged();
}


bool
PadView::RemoveButton(LaunchButton* button)
{
	bool result = fButtonLayout->RemoveView(button);
	if (result)
		_NotifySettingsChanged();
	return result;
}


LaunchButton*
PadView::ButtonAt(int32 index) const
{
	BLayoutItem* item = fButtonLayout->ItemAt(index);
	if (item == NULL)
		return NULL;
	return dynamic_cast<LaunchButton*>(item->View());
}


void
PadView::DisplayMenu(BPoint where, LaunchButton* button) const
{
	MainWindow* window = dynamic_cast<MainWindow*>(Window());
	if (window == NULL)
		return;

	LaunchButton* nearestButton = button;
	if (!nearestButton) {
		// find the nearest button
		for (int32 i = 0; (nearestButton = ButtonAt(i)); i++) {
			if (nearestButton->Frame().top > where.y)
				break;
		}
	}

	BPopUpMenu* menu = new BPopUpMenu(B_TRANSLATE("launch popup"), false, false);

	// Disabled menu item showing the name of the app... (because it provides no clues otherwise)
	BMenuItem* item = new BMenuItem(B_TRANSLATE("LaunchBox"), NULL);
	item->SetEnabled(false);
	menu->AddItem(item);

	menu->AddSeparatorItem();

	// Pad related menu items...
	item = new BMenuItem(B_TRANSLATE("New pad" B_UTF8_ELLIPSIS), new BMessage(MSG_ADD_WINDOW));
	item->SetTarget(be_app);
	menu->AddItem(item);
	item = new BMenuItem(B_TRANSLATE("Clone pad" B_UTF8_ELLIPSIS), new BMessage(MSG_ADD_WINDOW));
	item->SetTarget(window);
	menu->AddItem(item);
	item = new BMenuItem(B_TRANSLATE("Close pad" B_UTF8_ELLIPSIS), new BMessage(B_QUIT_REQUESTED));
	item->SetTarget(window);
	int32 padWindowCount = 0;
	for (int32 i = 0; BWindow* window = be_app->WindowAt(i); i++) {
		if (dynamic_cast<MainWindow*>(window))
			padWindowCount++;
	}
	if (padWindowCount == 1)
		item->SetEnabled(false);
	menu->AddItem(item);

	menu->AddSeparatorItem();

	// Button related menu items...
	BMessage* message = new BMessage(MSG_ADD_SLOT);
	message->AddPointer("be:source", (void*)nearestButton);
	item = new BMenuItem(B_TRANSLATE("Add button here" B_UTF8_ELLIPSIS), message);
	item->SetTarget(window);
	menu->AddItem(item);

	// Options for previously added buttons...
	if (button) {

		// Clear button
		if (button->Ref()) {
			message = new BMessage(MSG_CLEAR_SLOT);
			message->AddPointer("be:source", (void*)button);
			item = new BMenuItem(B_TRANSLATE("Clear button" B_UTF8_ELLIPSIS), message);
			item->SetTarget(window);
			menu->AddItem(item);
		}

		// Remove button
		message = new BMessage(MSG_REMOVE_SLOT);
		message->AddPointer("be:source", (void*)button);
		item = new BMenuItem(B_TRANSLATE("Remove button" B_UTF8_ELLIPSIS), message);
		item->SetTarget(window);
		menu->AddItem(item);

		// Open the button's containing folder
		if (button->Ref() != NULL) {
			message = new BMessage(MSG_OPEN_CONTAINING_FOLDER);
			message->AddPointer("be:source", (void*)button);
			item = new BMenuItem(B_TRANSLATE("Open containing folder" B_UTF8_ELLIPSIS), message);
			item->SetTarget(window);
			menu->AddItem(item);
		}

		// Set button description
		if (button->Ref()) {
			message = new BMessage(MSG_SET_DESCRIPTION);
			message->AddPointer("be:source", (void*)button);
			item = new BMenuItem(B_TRANSLATE("Set description" B_UTF8_ELLIPSIS),
				message);
			item->SetTarget(window);
			menu->AddItem(item);
		}
	}

	menu->AddSeparatorItem();

	// Toggle between horizontal and vertical orientation...
	const char* toggleLayoutLabel;
	if (fButtonLayout->Orientation() == B_HORIZONTAL)
		toggleLayoutLabel = B_TRANSLATE("Vertical layout" B_UTF8_ELLIPSIS);
	else
		toggleLayoutLabel = B_TRANSLATE("Horizontal layout" B_UTF8_ELLIPSIS);
	item = new BMenuItem(toggleLayoutLabel, new BMessage(MSG_TOGGLE_LAYOUT));
	item->SetTarget(this);
	menu->AddItem(item);

	// Submenu to set icon size for the pad...
	BMenu* iconSizeMenu = new BMenu(B_TRANSLATE("Icon size"));
	for (uint32 i = 0; i < sizeof(kIconSizes) / sizeof(uint32); i++) {
		uint32 iconSize = kIconSizes[i];
		message = new BMessage(MSG_SET_ICON_SIZE);
		message->AddInt32("size", iconSize);
		BString label;
		label << iconSize << " x " << iconSize;
		item = new BMenuItem(label.String(), message);
		item->SetTarget(this);
		item->SetMarked(IconSize() == iconSize);
		iconSizeMenu->AddItem(item);
	}
	menu->AddItem(iconSizeMenu);

	// Submenu for pad window related settings...
	BMenu* settingsSubMenu = new BMenu(B_TRANSLATE("Other settings"));

	uint32 what = window->Look() == B_BORDERED_WINDOW_LOOK ? MSG_SHOW_BORDER : MSG_HIDE_BORDER;
	item = new BMenuItem(B_TRANSLATE("Show window tab & border"), new BMessage(what));
	item->SetTarget(window);
	item->SetMarked(what == MSG_HIDE_BORDER);
	settingsSubMenu->AddItem(item);

	settingsSubMenu->AddSeparatorItem();

	item = new BMenuItem(B_TRANSLATE("Ignore double-click"),
		new BMessage(MSG_SET_IGNORE_DOUBLECLICK));
	item->SetTarget(this);
	item->SetMarked(IgnoreDoubleClick());
	settingsSubMenu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Autostart"), new BMessage(MSG_TOGGLE_AUTOSTART));
	item->SetTarget(be_app);
	item->SetMarked(((App*)be_app)->AutoStart());
	settingsSubMenu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Auto-raise"), new BMessage(MSG_TOGGLE_AUTORAISE));
	item->SetTarget(window);
	item->SetMarked(window->AutoRaise());
	settingsSubMenu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Show on all workspaces"), new BMessage(MSG_SHOW_ON_ALL_WORKSPACES));
	item->SetTarget(window);
	item->SetMarked(window->ShowOnAllWorkspaces());
	settingsSubMenu->AddItem(item);

	menu->AddItem(settingsSubMenu);

	menu->AddSeparatorItem();

	// Quit LaunchBox menu item...
	item = new BMenuItem(B_TRANSLATE("Quit" B_UTF8_ELLIPSIS), new BMessage(B_QUIT_REQUESTED));
	item->SetTarget(be_app);
	menu->AddItem(item);

	// Finish and show the popup menu...
	menu->SetAsyncAutoDestruct(true);
	where = ConvertToScreen(where);
	BRect mouseRect(where, where);
	mouseRect.InsetBy(-4.0, -4.0);
	menu->Go(where, true, false, mouseRect, true);
}


void
PadView::SetOrientation(enum orientation orientation)
{
	if (orientation == B_VERTICAL) {
		fButtonLayout->SetInsets(2, 7, 2, 2);
		fButtonLayout->SetOrientation(B_VERTICAL);
	} else {
		fButtonLayout->SetInsets(7, 2, 2, 2);
		fButtonLayout->SetOrientation(B_HORIZONTAL);
	}
	_NotifySettingsChanged();
}


enum orientation
PadView::Orientation() const
{
	return fButtonLayout->Orientation();
}


void
PadView::SetIconSize(uint32 size)
{
	if (size == fIconSize)
		return;

	fIconSize = size;

	for (int32 i = 0; LaunchButton* button = ButtonAt(i); i++)
		button->SetIconSize(fIconSize);

	_NotifySettingsChanged();
}


uint32
PadView::IconSize() const
{
	return fIconSize;
}


void
PadView::SetIgnoreDoubleClick(bool refuse)
{
	LaunchButton::SetIgnoreDoubleClick(refuse);

	_NotifySettingsChanged();
}


bool
PadView::IgnoreDoubleClick() const
{
	return LaunchButton::IgnoreDoubleClick();
}


void
PadView::_NotifySettingsChanged()
{
	be_app->PostMessage(MSG_SETTINGS_CHANGED);
}
