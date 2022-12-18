/*
 * Copyright 2022 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ThemeView.h"

#include <stdio.h>

#include <Alert.h>
#include <Catalog.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Messenger.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <SpaceLayoutItem.h>

#include "ThemeWindow.h"
#include "Colors.h"
#include "ColorPreview.h"
#include "ColorListView.h"
#include "ColorItem.h"
#include "TermConst.h"
#include "PrefHandler.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Colors tab"

#define COLOR_DROPPED 'cldp'
#define DECORATOR_CHANGED 'dcch'


/* static */ const char*
ThemeView::kColorTable[] = {
		"Text",
		"Background",
		"Cursor",
		"Text under cursor",
		"Selected text",
		"Selected background",
		"ANSI black color",
		"ANSI red color",
		"ANSI green color",
		"ANSI yellow color",
		"ANSI blue color",
		"ANSI magenta color",
		"ANSI cyan color",
		"ANSI white color",
		"ANSI bright black color",
		"ANSI bright red color",
		"ANSI bright green color",
		"ANSI bright yellow color",
		"ANSI bright blue color",
		"ANSI bright magenta color",
		"ANSI bright cyan color",
		"ANSI bright white color",
		NULL
};


ThemeView::ThemeView(const char* name, const BMessenger& messenger)
	:
	BGroupView(name, B_VERTICAL, 5),
	fTerminalMessenger(messenger)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	// Set up list of color attributes
	fAttrList = new ColorListView("AttributeList");

	fScrollView = new BScrollView("ScrollView", fAttrList, 0, false, true);
	fScrollView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	PrefHandler* prefHandler = PrefHandler::Default();

	for (const char** table = kColorTable; *table != NULL; ++table) {
		fAttrList->AddItem(new ColorItem(*table, prefHandler->getRGB(*table)));
	}

	BPopUpMenu* schemesPopUp = _MakeColorSchemeMenu(MSG_COLOR_SCHEME_CHANGED,
		gColorSchemes);
	fColorSchemeField = new BMenuField(B_TRANSLATE("Color scheme:"),
		schemesPopUp);

	fColorPreview = new ColorPreview(new BMessage(COLOR_DROPPED), 0);
	fColorPreview->SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER,
		B_ALIGN_VERTICAL_CENTER));

	fPicker = new BColorControl(B_ORIGIN, B_CELLS_32x8, 8.0,
		"picker", new BMessage(MSG_UPDATE_COLOR));
	
	fPreview = new BTextView("preview");

	BLayoutBuilder::Group<>(this)
		.SetInsets(5, 5, 5, 5)
		.AddGrid()
			.Add(fColorSchemeField->CreateLabelLayoutItem(), 0, 5)
			.Add(fColorSchemeField->CreateMenuBarLayoutItem(), 1, 5)
			.End()
		.AddGlue()
		.Add(fScrollView, 10.0)
		.Add(fPreview)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(fColorPreview)
			.AddGlue()
			.Add(fPicker);

	fColorPreview->Parent()->SetExplicitMaxSize(
		BSize(B_SIZE_UNSET, fPicker->Bounds().Height()));
	fAttrList->SetSelectionMessage(new BMessage(MSG_COLOR_ATTRIBUTE_CHOSEN));
	fScrollView->SetExplicitMinSize(BSize(B_SIZE_UNSET, 22 * 16));
	fColorSchemeField->SetAlignment(B_ALIGN_RIGHT);

	_UpdateStyle();

	InvalidateLayout(true);
}


ThemeView::~ThemeView()
{
}


void
ThemeView::_UpdateStyle()
{
	PrefHandler* prefHandler = PrefHandler::Default();

	text_run_array *array = (text_run_array*)malloc(sizeof(text_run_array)
			+ sizeof(text_run) * 18);
	array->count = 18;
	array->runs[0].offset = 0;
	array->runs[0].font = *be_fixed_font;
	array->runs[0].color = prefHandler->getRGB(PREF_TEXT_FORE_COLOR);
	array->runs[1].offset = 25;
	array->runs[1].font = *be_fixed_font;
	array->runs[1].color = prefHandler->getRGB(PREF_ANSI_BLACK_COLOR);
	array->runs[2].offset = 32;
	array->runs[2].font = *be_fixed_font;
	array->runs[2].color = prefHandler->getRGB(PREF_ANSI_RED_COLOR);
	array->runs[3].offset = 36;
	array->runs[3].font = *be_fixed_font;
	array->runs[3].color = prefHandler->getRGB(PREF_ANSI_GREEN_COLOR);
	array->runs[4].offset = 42;
	array->runs[4].font = *be_fixed_font;
	array->runs[4].color = prefHandler->getRGB(PREF_ANSI_YELLOW_COLOR);
	array->runs[5].offset = 49;
	array->runs[5].font = *be_fixed_font;
	array->runs[5].color = prefHandler->getRGB(PREF_ANSI_BLUE_COLOR);
	array->runs[6].offset = 54;
	array->runs[6].font = *be_fixed_font;
	array->runs[6].color = prefHandler->getRGB(PREF_ANSI_MAGENTA_COLOR);
	array->runs[7].offset = 62;
	array->runs[7].font = *be_fixed_font;
	array->runs[7].color = prefHandler->getRGB(PREF_ANSI_CYAN_COLOR);
	array->runs[8].offset = 67;
	array->runs[8].font = *be_fixed_font;
	array->runs[8].color = prefHandler->getRGB(PREF_ANSI_WHITE_COLOR);
	array->runs[9].offset = 72;
	array->runs[9].font = *be_fixed_font;
	array->runs[9].color = prefHandler->getRGB(PREF_TEXT_FORE_COLOR);
	array->runs[10].offset = 82;
	array->runs[10].font = *be_fixed_font;
	array->runs[10].color = prefHandler->getRGB(PREF_ANSI_BLACK_HCOLOR);
	array->runs[11].offset = 87;
	array->runs[11].font = *be_fixed_font;
	array->runs[11].color = prefHandler->getRGB(PREF_ANSI_RED_HCOLOR);
	array->runs[12].offset = 92;
	array->runs[12].font = *be_fixed_font;
	array->runs[12].color = prefHandler->getRGB(PREF_ANSI_GREEN_HCOLOR);
	array->runs[13].offset = 98;
	array->runs[13].font = *be_fixed_font;
	array->runs[13].color = prefHandler->getRGB(PREF_ANSI_YELLOW_HCOLOR);
	array->runs[14].offset = 105;
	array->runs[14].font = *be_fixed_font;
	array->runs[14].color = prefHandler->getRGB(PREF_ANSI_BLUE_HCOLOR);
	array->runs[15].offset = 110;
	array->runs[15].font = *be_fixed_font;
	array->runs[15].color = prefHandler->getRGB(PREF_ANSI_MAGENTA_HCOLOR);
	array->runs[16].offset = 118;
	array->runs[16].font = *be_fixed_font;
	array->runs[16].color = prefHandler->getRGB(PREF_ANSI_CYAN_HCOLOR);
	array->runs[17].offset = 123;
	array->runs[17].font = *be_fixed_font;
	array->runs[17].color = prefHandler->getRGB(PREF_ANSI_WHITE_HCOLOR);

	fPreview->SetStylable(true);
	fPreview->MakeEditable(false);
	fPreview->MakeSelectable(false);
	fPreview->SetViewColor(prefHandler->getRGB(PREF_TEXT_BACK_COLOR));
	fPreview->SetText(
		" foreground text\n"
		" normal: black red green yellow blue magenta cyan white\n"
		" bright: black red green yellow blue magenta cyan white", array);
	font_height height;
	be_fixed_font->GetHeight(&height);
	fPreview->SetExplicitMinSize(BSize(B_SIZE_UNSET, (height.ascent + height.descent) * 3));
	fPreview->Invalidate();
}


void
ThemeView::AttachedToWindow()
{
	fPicker->SetTarget(this);
	fAttrList->SetTarget(this);
	fColorPreview->SetTarget(this);
	fColorSchemeField->Menu()->SetTargetForItems(this);

	fAttrList->Select(0);
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	_SetCurrentColorScheme();
}


void
ThemeView::MessageReceived(BMessage *msg)
{
	bool modified = false;

	switch (msg->what) {
		case MSG_COLOR_SCHEME_CHANGED:
		{
			color_scheme* newScheme = NULL;
			if (msg->FindPointer("color_scheme",
					(void**)&newScheme) == B_OK) {
				_ChangeColorScheme(newScheme);
				modified = true;
			}
			break;
		}

		case MSG_SET_CURRENT_COLOR:
		{
			rgb_color* color;
			ssize_t size;

			if (msg->FindData(kRGBColor, B_RGB_COLOR_TYPE,
					(const void**)&color, &size) == B_OK) {
				_SetCurrentColor(*color);
				modified = true;
			}
			break;
		}

		case MSG_UPDATE_COLOR:
		{
			// Received from the color fPicker when its color changes
			rgb_color color = fPicker->ValueAsColor();
			_SetCurrentColor(color);
			modified = true;

			break;
		}

		case MSG_COLOR_ATTRIBUTE_CHOSEN:
		{
			// Received when the user chooses a GUI fAttribute from the list
			ColorItem* item = (ColorItem*)
				fAttrList->ItemAt(fAttrList->CurrentSelection());
			if (item == NULL)
				break;

			const char* label = item->Text();
			rgb_color color = PrefHandler::Default()->getRGB(label);
			_SetCurrentColor(color);
			break;
		}

		default:
			BView::MessageReceived(msg);
			break;
	}

	if (modified) {
		_UpdateStyle();
		fTerminalMessenger.SendMessage(msg);

		BMessenger messenger(this);
		messenger.SendMessage(MSG_THEME_MODIFIED);
	}
}


void
ThemeView::SetDefaults()
{
	PrefHandler* prefHandler = PrefHandler::Default();

	int32 count = fAttrList->CountItems();
	for (int32 index = 0; index < count; ++index) {
		ColorItem* item = (ColorItem*)fAttrList->ItemAt(index);
		item->SetColor(prefHandler->getRGB(item->Text()));
		fAttrList->InvalidateItem(index);
	}

	int32 currentIndex = fAttrList->CurrentSelection();
	ColorItem* item = (ColorItem*)fAttrList->ItemAt(currentIndex);
	if (item != NULL) {
		rgb_color color = item->Color();
		fPicker->SetValue(color);
		fColorPreview->SetColor(color);
		fColorPreview->Invalidate();
	}

	_UpdateStyle();

	BMessage message(MSG_COLOR_SCHEME_CHANGED);
	fTerminalMessenger.SendMessage(&message);
}


void
ThemeView::Revert()
{
	_SetCurrentColorScheme();

	SetDefaults();
}


void
ThemeView::_ChangeColorScheme(color_scheme* scheme)
{
	PrefHandler* pref = PrefHandler::Default();

	pref->setRGB(PREF_TEXT_FORE_COLOR, scheme->text_fore_color);
	pref->setRGB(PREF_TEXT_BACK_COLOR, scheme->text_back_color);
	pref->setRGB(PREF_SELECT_FORE_COLOR, scheme->select_fore_color);
	pref->setRGB(PREF_SELECT_BACK_COLOR, scheme->select_back_color);
	pref->setRGB(PREF_CURSOR_FORE_COLOR, scheme->cursor_fore_color);
	pref->setRGB(PREF_CURSOR_BACK_COLOR, scheme->cursor_back_color);
	pref->setRGB(PREF_ANSI_BLACK_COLOR, scheme->ansi_colors.black);
	pref->setRGB(PREF_ANSI_RED_COLOR, scheme->ansi_colors.red);
	pref->setRGB(PREF_ANSI_GREEN_COLOR, scheme->ansi_colors.green);
	pref->setRGB(PREF_ANSI_YELLOW_COLOR, scheme->ansi_colors.yellow);
	pref->setRGB(PREF_ANSI_BLUE_COLOR, scheme->ansi_colors.blue);
	pref->setRGB(PREF_ANSI_MAGENTA_COLOR, scheme->ansi_colors.magenta);
	pref->setRGB(PREF_ANSI_CYAN_COLOR, scheme->ansi_colors.cyan);
	pref->setRGB(PREF_ANSI_WHITE_COLOR, scheme->ansi_colors.white);
	pref->setRGB(PREF_ANSI_BLACK_HCOLOR, scheme->ansi_colors_h.black);
	pref->setRGB(PREF_ANSI_RED_HCOLOR, scheme->ansi_colors_h.red);
	pref->setRGB(PREF_ANSI_GREEN_HCOLOR, scheme->ansi_colors_h.green);
	pref->setRGB(PREF_ANSI_YELLOW_HCOLOR, scheme->ansi_colors_h.yellow);
	pref->setRGB(PREF_ANSI_BLUE_HCOLOR, scheme->ansi_colors_h.blue);
	pref->setRGB(PREF_ANSI_MAGENTA_HCOLOR, scheme->ansi_colors_h.magenta);
	pref->setRGB(PREF_ANSI_CYAN_HCOLOR, scheme->ansi_colors_h.cyan);
	pref->setRGB(PREF_ANSI_WHITE_HCOLOR, scheme->ansi_colors_h.white);

	int32 count = fAttrList->CountItems();
	for (int32 index = 0; index < count; ++index)
	{
		ColorItem* item = static_cast<ColorItem*>(fAttrList->ItemAt(index));
		rgb_color color = pref->getRGB(item->Text());
		item->SetColor(color);

		if (item->IsSelected()) {
			fPicker->SetValue(color);
			fColorPreview->SetColor(color);
			fColorPreview->Invalidate();
		}
	}

	fAttrList->Invalidate();
}


void
ThemeView::_SetCurrentColorScheme()
{
	PrefHandler* pref = PrefHandler::Default();

	pref->LoadColorScheme(&gCustomColorScheme);

	const char* currentSchemeName = NULL;

	int32 i = 0;
	while (i < gColorSchemes->CountItems()) {
		const color_scheme *item = gColorSchemes->ItemAt(i);
		i++;

		if (gCustomColorScheme == *item) {
			currentSchemeName = item->name;
			break;
		}
	}

	// If the scheme is not one of the known ones, assume a custom one.
	if (currentSchemeName == NULL)
		currentSchemeName = "Custom";

	for (int32 i = 0; i < fColorSchemeField->Menu()->CountItems(); i++) {
		BMenuItem* item = fColorSchemeField->Menu()->ItemAt(i);
		if (strcmp(item->Label(), currentSchemeName) == 0) {
			item->SetMarked(true);
			break;
		}
	}
}


/*static*/ void
ThemeView::_MakeColorSchemeMenuItem(uint32 msg, const color_scheme *item,
	BPopUpMenu *menu)
{
	if (item == NULL)
		return;

	BMessage* message = new BMessage(msg);
	message->AddPointer("color_scheme", (const void*)item);
	menu->AddItem(new BMenuItem(item->name, message));
}


/*static*/ BPopUpMenu*
ThemeView::_MakeColorSchemeMenu(uint32 msg, BObjectList<const color_scheme> *items)
{
	BPopUpMenu* menu = new BPopUpMenu("");

	FindColorSchemeByName comparator("Default");

	const color_scheme* defaultItem = items->FindIf(comparator);

	_MakeColorSchemeMenuItem(msg, defaultItem, menu);

	int32 i = 0;
	while (i < items->CountItems()) {
		const color_scheme *item = items->ItemAt(i);
		i++;

		if (strcmp(item->name, "") == 0)
			menu->AddSeparatorItem();
		else if (item == defaultItem)
			continue;
		else
			_MakeColorSchemeMenuItem(msg, item, menu);
	}

	// Add the custom item at the very end
	menu->AddSeparatorItem();
	_MakeColorSchemeMenuItem(msg, &gCustomColorScheme, menu);

	return menu;
}


void
ThemeView::_SetCurrentColor(rgb_color color)
{
	int32 currentIndex = fAttrList->CurrentSelection();
	ColorItem* item = (ColorItem*)fAttrList->ItemAt(currentIndex);
	if (item != NULL) {
		item->SetColor(color);
		fAttrList->InvalidateItem(currentIndex);

		PrefHandler::Default()->setRGB(item->Text(), color);
	}

	fPicker->SetValue(color);
	fColorPreview->SetColor(color);
	fColorPreview->Invalidate();
}
