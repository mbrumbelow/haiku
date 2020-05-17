/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
*/


#include "InputDeviceView.h"


#include <Catalog.h>
#include <DateFormat.h>
#include <Input.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <Locale.h>
#include <ScrollView.h>
#include <String.h>
#include <StringItem.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DeviceList"


InputIcons* DeviceListItemView::sIcons = NULL;

DeviceListItemView::DeviceListItemView(const char* title, input_type type)
	:
	BListItem((uint32)0),
	fTitle(title),
	fInputType(type)
{
}

struct DeviceListItemView::Renderer {
	Renderer()
		:
		fTitle(NULL),
		fPrimaryIcon(NULL),
		fSecondaryIcon(NULL),
		fDoubleInsets(true),
		fSelected(false)
	{
	}

	void AddIcon(BBitmap* icon)
	{
		if (!fPrimaryIcon)
			fPrimaryIcon = icon;
		else {
			fSecondaryIcon = fPrimaryIcon;
			fPrimaryIcon = icon;
		}
	}

	void SetTitle(const char* title)
	{
		fTitle = title;
	}

	void SetSelected(bool selected)
	{
		fSelected = selected;
	}

	void UseDoubleInset(bool doubleInset)
	{
		fDoubleInsets = doubleInset;
	}

	void Render(BView* onto, BRect frame, bool complete = false)
	{
		const rgb_color lowColor = onto->LowColor();
		const rgb_color highColor = onto->HighColor();

		if (fSelected || complete) {
			if (fSelected)
				onto->SetLowColor(ui_color(B_LIST_SELECTED_BACKGROUND_COLOR));
			onto->FillRect(frame, B_SOLID_LOW);
		}

		BPoint point(frame.left + 4.0f,
			frame.top + (frame.Height() - InputIcons::sBounds.Height()) / 2.0f);

		BRect iconFrame(InputIcons::IconRectAt(point + BPoint(1, 0)));

		onto->SetDrawingMode(B_OP_OVER);
		if (fPrimaryIcon && !fDoubleInsets) {
			onto->DrawBitmap(fPrimaryIcon, iconFrame);
			point.x = iconFrame.right + 1;
		} else if (fSecondaryIcon) {
			onto->DrawBitmap(fSecondaryIcon, iconFrame);
		}

		iconFrame = InputIcons::IconRectAt(iconFrame.RightTop() + BPoint(1, 0));

		if (fDoubleInsets) {
			if (fPrimaryIcon != NULL)
				onto->DrawBitmap(fPrimaryIcon, iconFrame);
			point.x = iconFrame.right + 1;
		}

		onto->SetDrawingMode(B_OP_COPY);

		BFont font = be_plain_font;
		font_height	fontInfo;
		font.GetHeight(&fontInfo);

		onto->SetFont(&font);
		onto->MovePenTo(point.x + 8, frame.top
			+ fontInfo.ascent + (frame.Height()
			- ceilf(fontInfo.ascent + fontInfo.descent)) / 2.0f);
		onto->DrawString(fTitle);

		onto->SetHighColor(highColor);
		onto->SetLowColor(lowColor);
	}

	float ItemWidth()
	{
		float width = 4.0f;

		float iconSpace = InputIcons::sBounds.Width() + 1.0f;
		if (fDoubleInsets)
			iconSpace *= 2.0f;
		width += iconSpace;
		width += 8.0f;

		width += be_plain_font->StringWidth(fTitle) + 16.0f;
		return width;
	}
private:

	const char*	fTitle;
	BBitmap*	fPrimaryIcon;
	BBitmap*	fSecondaryIcon;
	bool		fDoubleInsets;
	bool		fSelected;
};

void
DeviceListItemView::Update(BView* owner, const BFont* font)
{
	BListItem::Update(owner, font);

	float iconHeight = InputIcons::sBounds.Height() + 1;
	if ((Height() < iconHeight + kITEM_MARGIN * 2)) {
		SetHeight(iconHeight + kITEM_MARGIN * 2);
	}

	Renderer renderer;
	renderer.SetTitle(Label());
	renderer.SetTitle(fTitle);
	SetRenderParameters(renderer);
	SetWidth(renderer.ItemWidth());
};

void
DeviceListItemView::DrawItem(BView* owner, BRect frame, bool complete)
{
	Renderer renderer;
	renderer.SetSelected(IsSelected());
	renderer.SetTitle(Label());
	SetRenderParameters(renderer);
	renderer.Render(owner, frame, complete);
};

void
DeviceListItemView::SetRenderParameters(Renderer& renderer)
{
	renderer.AddIcon(&Icons()->devicesIcon);
	if (fInputType == input_type::MOUSE_TYPE){
		InputIcons::IconSet* iconSet = &Icons()->mouseIcons;
		iconSet = &Icons()->mouseIcons;
	}
	else if (fInputType == input_type::TOUCHPAD_TYPE) {
		InputIcons::IconSet* iconSet = &Icons()->touchpadIcons;
		iconSet = &Icons()->touchpadIcons;
	}
	else if (fInputType == input_type::KEYBOARD_TYPE){
		InputIcons::IconSet* iconSet = &Icons()->keyboardIcons;
		iconSet = &Icons()->keyboardIcons;
	}
}


DeviceListView::DeviceListView(const char* name)
	:
	BView(name, B_WILL_DRAW)
{
	fDeviceList = new BListView("Device Names");

	fScrollView = new BScrollView("ScrollView",fDeviceList,
					0 , false, B_FANCY_BORDER);

	SetExplicitMinSize(BSize(StringWidth("M") * 10, B_SIZE_UNSET));
	SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(fScrollView)
		.End();
	fDeviceList->SetSelectionMessage(new BMessage(ITEM_SELECTED));
}

DeviceListView::~DeviceListView()
{
}

void
DeviceListView::AttachedToWindow()
{
	fDeviceList->SetTarget(this);

	fDeviceList->Select(0);
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
}
