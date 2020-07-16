/*
 * Copyright 2010-2017, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 */


#include "ConfigView.h"

#include <stdio.h>
#include <string.h>

#include <Catalog.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <Slider.h>
#include <StringView.h>

#include "avif/avif.h"

#include "TranslatorSettings.h"
#include "AVIFTranslator.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ConfigView"


static const uint32 kMsgQuality	= 'qlty';
static const uint32 kMsgMethod	= 'metd';
static const uint32 kMsgPreprocessing = 'pprc';


ConfigView::ConfigView(TranslatorSettings* settings)
	:
	BGroupView(B_TRANSLATE("AVIFTranslator Settings"), B_VERTICAL),
	fSettings(settings)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	BStringView* title = new BStringView("title",
		B_TRANSLATE("AVIF image translator"));
	title->SetFont(be_bold_font);

	char versionString[256];
	sprintf(versionString, "v%d.%d.%d, %s",
		static_cast<int>(B_TRANSLATION_MAJOR_VERSION(AVIF_TRANSLATOR_VERSION)),
		static_cast<int>(B_TRANSLATION_MINOR_VERSION(AVIF_TRANSLATOR_VERSION)),
		static_cast<int>(B_TRANSLATION_REVISION_VERSION(
			AVIF_TRANSLATOR_VERSION)),
		__DATE__);

	BStringView* version = new BStringView("version", versionString);

	BString copyrightsText;
	BStringView *copyrightView = new BStringView("Copyright",
		B_TRANSLATE(B_UTF8_COPYRIGHT "2020 Emmanuel Gil Peyrot"));

	BString libavifInfo = B_TRANSLATE(
		"Based on libavif %version%");
	libavifInfo.ReplaceAll("%version%", avifVersion());
	
	BStringView *copyright2View = new BStringView("Copyright2",
		libavifInfo.String());
	BStringView *copyright3View = new BStringView("Copyright3",
		B_TRANSLATE(B_UTF8_COPYRIGHT "2019 Joe Drago. All rights reserved."));

	// output parameters

	fQualitySlider = new BSlider("quality", B_TRANSLATE("Output quality:"),
		new BMessage(kMsgQuality), 0, 100, B_HORIZONTAL, B_BLOCK_THUMB);
	fQualitySlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fQualitySlider->SetHashMarkCount(10);
	fQualitySlider->SetLimitLabels(B_TRANSLATE("Low"), B_TRANSLATE("High"));
	fQualitySlider->SetValue(fSettings->SetGetInt32(AVIF_SETTING_QUALITY));

	fMethodSlider = new BSlider("method", B_TRANSLATE("Compression method:"),
		new BMessage(kMsgMethod), 0, 6, B_HORIZONTAL, B_BLOCK_THUMB);
	fMethodSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fMethodSlider->SetHashMarkCount(7);
	fMethodSlider->SetLimitLabels(B_TRANSLATE("Fast"),
		B_TRANSLATE("Slower but better"));
	fMethodSlider->SetValue(fSettings->SetGetInt32(AVIF_SETTING_METHOD));

	fPreprocessingCheckBox = new BCheckBox("preprocessing",
		B_TRANSLATE("Preprocessing filter"), new BMessage(kMsgPreprocessing));
	if (fSettings->SetGetBool(AVIF_SETTING_PREPROCESSING))
		fPreprocessingCheckBox->SetValue(B_CONTROL_ON);

	// Build the layout
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(title)
		.Add(version)
		.Add(copyrightView)
		.AddGlue()
		.Add(fQualitySlider)
		.Add(fMethodSlider)
		.Add(fPreprocessingCheckBox)
		.AddGlue()
		.Add(copyright2View)
		.Add(copyright3View);

	BFont font;
	GetFont(&font);
	SetExplicitPreferredSize(BSize((font.Size() * 250) / 12,
		(font.Size() * 350) / 12));
}


ConfigView::~ConfigView()
{
	fSettings->Release();
}


void
ConfigView::AttachedToWindow()
{
	BGroupView::AttachedToWindow();

	fQualitySlider->SetTarget(this);
	fMethodSlider->SetTarget(this);
	fPreprocessingCheckBox->SetTarget(this);

	if (Parent() == NULL && Window()->GetLayout() == NULL) {
		Window()->SetLayout(new BGroupLayout(B_VERTICAL));
		Window()->ResizeTo(PreferredSize().Width(), PreferredSize().Height());
	}
}


void
ConfigView::MessageReceived(BMessage* message)
{
	struct {
		const char*		name;
		uint32			what;
		TranSettingType	type;
	} maps[] = {
		{ AVIF_SETTING_QUALITY, kMsgQuality, TRAN_SETTING_INT32 },
		{ AVIF_SETTING_METHOD, kMsgMethod, TRAN_SETTING_INT32 },
		{ AVIF_SETTING_PREPROCESSING, kMsgPreprocessing, TRAN_SETTING_BOOL },
		{ NULL }
	};

	int i;
	for (i = 0; maps[i].name != NULL; i++) {
		if (maps[i].what == message->what)
			break;
	}

	if (maps[i].name == NULL) {
		BGroupView::MessageReceived(message);
		return;
	}

	int32 value;
	if (message->FindInt32("value", &value) == B_OK
		|| message->FindInt32("be:value", &value) == B_OK) {
		switch(maps[i].type) {
			case TRAN_SETTING_BOOL:
			{
				bool boolValue = value;
				fSettings->SetGetBool(maps[i].name, &boolValue);
				break;
			}
			case TRAN_SETTING_INT32:
				fSettings->SetGetInt32(maps[i].name, &value);
				break;
		}
		fSettings->SaveSettings();
	}
}
