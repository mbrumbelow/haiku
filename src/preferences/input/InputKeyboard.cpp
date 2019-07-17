/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#include "InputKeyboard.h"

#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Message.h>
#include <SeparatorView.h>
#include <Slider.h>
#include <TextControl.h>

#include "InputConstants.h"
#include "KeyboardView.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InputKeyboard"

InputKeyboard::InputKeyboard(DeviceListView* devicelistview)
	:
	BView("InputKeyboard", B_WILL_DRAW)
{
	fConnected = false;
	fDeviceListView = devicelistview;
	ConnectToKeyboard();
	// Add the main settings view
	fSettingsView = new KeyboardView();

	// Add the "Default" button..
	fDefaultsButton = new BButton(B_TRANSLATE("Defaults"),
        new BMessage(BUTTON_DEFAULTS));

	// Add the "Revert" button...
	fRevertButton = new BButton(B_TRANSLATE("Revert"),
        new BMessage(BUTTON_REVERT));
	fRevertButton->SetEnabled(false);

	// Build the layout
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.AddGroup(B_HORIZONTAL)
			.SetInsets(B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING,
				B_USE_WINDOW_SPACING, 0)
			.Add(fSettingsView)
			.End()
		.Add(new BSeparatorView(B_HORIZONTAL))
		.AddGroup(B_HORIZONTAL)
			.SetInsets(B_USE_WINDOW_SPACING, 0, B_USE_WINDOW_SPACING,
				B_USE_WINDOW_SPACING)
			.Add(fDefaultsButton)
			.Add(fRevertButton)
			.End();

	BSlider* slider = (BSlider* )FindView("key_repeat_rate");
	if (slider !=NULL)
		slider->SetValue(fSettings.KeyboardRepeatRate());

	slider = (BSlider* )FindView("delay_until_key_repeat");
	if (slider !=NULL)
		slider->SetValue(fSettings.KeyboardRepeatDelay());

	fDefaultsButton->SetEnabled(fSettings.IsDefaultable());
}

void
InputKeyboard::MessageReceived(BMessage* message)
{
	BSlider* slider = NULL;

	switch (message->what) {
		case BUTTON_DEFAULTS:
		{
			fSettings.Defaults();

			slider = (BSlider* )FindView("key_repeat_rate");
			if (slider !=NULL)
				slider->SetValue(fSettings.KeyboardRepeatRate());

			slider = (BSlider* )FindView("delay_until_key_repeat");
			if (slider !=NULL)
				slider->SetValue(fSettings.KeyboardRepeatDelay());

			fDefaultsButton->SetEnabled(false);

			fRevertButton->SetEnabled(true);
			break;
		}
		case BUTTON_REVERT:
		{
			fSettings.Revert();

			slider = (BSlider* )FindView("key_repeat_rate");
			if (slider !=NULL)
				slider->SetValue(fSettings.KeyboardRepeatRate());

			slider = (BSlider* )FindView("delay_until_key_repeat");
			if (slider !=NULL)
				slider->SetValue(fSettings.KeyboardRepeatDelay());

			fDefaultsButton->SetEnabled(fSettings.IsDefaultable());

			fRevertButton->SetEnabled(false);
			break;
		}
		case SLIDER_REPEAT_RATE:
		{
			int32 rate;
			if (message->FindInt32("be:value", &rate) != B_OK)
				break;
			fSettings.SetKeyboardRepeatRate(rate);

			fDefaultsButton->SetEnabled(fSettings.IsDefaultable());

			fRevertButton->SetEnabled(true);
			break;
		}
		case SLIDER_DELAY_RATE:
		{
			int32 delay;
			if (message->FindInt32("be:value", &delay) != B_OK)
				break;

			// We need to look at the value from the slider and make it "jump"
			// to the next notch along. Setting the min and max values of the
			// slider to 1 and 4 doesn't work like the real Keyboard app.
			if (delay < 375000)
				delay = 250000;
			if (delay >= 375000 && delay < 625000)
				delay = 500000;
			if (delay >= 625000 && delay < 875000)
				delay = 750000;
			if (delay >= 875000)
				delay = 1000000;

			fSettings.SetKeyboardRepeatDelay(delay);

			slider = (BSlider* )FindView("delay_until_key_repeat");
			if (slider != NULL)
				slider->SetValue(delay);

			fDefaultsButton->SetEnabled(fSettings.IsDefaultable());

			fRevertButton->SetEnabled(true);
			break;
	    }
    }
}

status_t
InputKeyboard::ConnectToKeyboard()
{
	BList devList;
	status_t status = get_input_devices(&devList);
	if (status != B_OK)
		return status;

	int32 i = 0;
	while (true) {
		BInputDevice* dev = (BInputDevice*)devList.ItemAt(i);
		if (dev == NULL)
			break;
		i++;

		LOG("input device %s\n", dev->Name());

		BString name = dev->Name();

		if (name.FindFirst("Keyboard") >= 0
			&& dev->Type() == B_KEYBOARD_DEVICE) {
			fConnected = true;
			fKeyboard = dev;
			fDeviceListView->fDeviceList->AddItem(new BStringItem(name));
			// Don't bail out here, since we need to delete the other devices
			// yet.
		} else {
			delete dev;
		}
	}
	if (fConnected)
		return B_OK;

	LOG("keyboard input device NOT found\n");
	return B_ENTRY_NOT_FOUND;
}