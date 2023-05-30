#include "AlarmSettings.h"

#include <stdio.h>
#include <string>

#include <Messenger.h>
#include <Application.h>
#include <View.h>
#include <Size.h>
#include <Rect.h>
#include <CheckBox.h>
#include <Message.h>
#include <Layout.h>
#include <Slider.h>
#include <Button.h>
#include <TextView.h>
#include <GroupLayout.h>
#include <SupportDefs.h>
#include <Alignment.h>
#include <OS.h>
#include <SeparatorView.h>

#include "Alarm.h"


using namespace std;


AlarmSettings::AlarmSettings(Alarm* alarm) 
	:
	BWindow(BRect(200, 200, 600, 550),
			"Alarm",
			B_FLOATING_WINDOW,
			B_CLOSE_ON_ESCAPE |
			B_NOT_RESIZABLE |
			B_WILL_ACCEPT_FIRST_CLICK),
	fAlarm(alarm)
{
	BView* background = new BView(Bounds(), NULL, B_FOLLOW_NONE, B_WILL_DRAW);
	AddChild(background);
	background->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	
	BGroupLayout* groupLayout = new BGroupLayout(B_VERTICAL);
	background->SetLayout(groupLayout);

	groupLayout->AddView(new BSeparatorView(B_HORIZONTAL, B_NO_BORDER));

	_MakeEnablerSoundsGroup(groupLayout);
	
	groupLayout->AddView(new BSeparatorView());
	
	_MakeCriteriaGroup(groupLayout);
		
	groupLayout->AddView(new BSeparatorView());

	_MakeRingsGroup(groupLayout);

	groupLayout->AddView(new BSeparatorView());

	_MakeIntervalGroup(groupLayout);

	groupLayout->AddView(new BSeparatorView(B_HORIZONTAL, B_NO_BORDER));

	_ToggleAdditionalInput();
	_UpdateShownValues();
}


AlarmSettings::~AlarmSettings()
{
}


BGroupLayout*
AlarmSettings::_MakeEnablerSoundsGroup(BGroupLayout* parent)
{
	BGroupLayout* group = new BGroupLayout(B_HORIZONTAL);
	parent->AddItem(group);

	group->AddView(_CreateTextBox("Enable alarm"));

	BCheckBox* enablerCheckBox = new BCheckBox(BRect(0, 0, 1, 1),
					NULL, NULL,
					new BMessage(kMsgToggleAlarm));
	enablerCheckBox->SetValue(fAlarm->IsActivated());
	group->AddView(enablerCheckBox);	
	fCheckBox = enablerCheckBox;

	group->AddView(new BSeparatorView(B_VERTICAL, B_PLAIN_BORDER));

	group->AddView(_CreateTextBox("Set audio"));

	BButton* selectSoundButton = new BButton(NULL, "Sounds..", 
			new BMessage(kMsgActivateSoundsApp));
	selectSoundButton->ResizeToPreferred();

	group->AddView(selectSoundButton);
	
	return group;
}


BGroupLayout*
AlarmSettings::_MakeCriteriaGroup(BGroupLayout* parent)
{
	return _CreateIntegerInputComponent(parent, "Set percentage for activation:",
			new BMessage(kMsgAlterAlarmCriteria), 1, 100, fAlarm->Criteria(),
			&fCriteriaSlider, &fCriteriaValText);
}


BGroupLayout*
AlarmSettings::_MakeRingsGroup(BGroupLayout* parent)
{
	return _CreateIntegerInputComponent(
		parent,
		"Set number of rings:",
		new BMessage(kMsgAlterAlarmRings),
		1, 100, fAlarm->Rings(),
		&fRingsSlider, &fRingsValText);
}


BGroupLayout*
AlarmSettings::_MakeIntervalGroup(BGroupLayout* parent)
{
	return _CreateIntegerInputComponent(
		parent,
		"Set interval between rings (ms):",
		new BMessage(kMsgAlterAlarmInterval),
		1, 1000, fAlarm->Interval(),
		&fIntervalSlider, &fIntervalValText);
}


BGroupLayout*
AlarmSettings::_CreateIntegerInputComponent(BGroupLayout* parent,
	const char* leftLabelContent, BMessage* sliderMessage,
	int32 sliderLowerBound, int32 sliderUpperBound,
	int32 sliderValue, BSlider** sliderAttribute,
	BTextView** valueTextAttribute)
{
	BGroupLayout* groupLayout = new BGroupLayout(B_HORIZONTAL);
	parent->AddItem(groupLayout);

	groupLayout->AddView(new BSeparatorView(B_VERTICAL, B_NO_BORDER));

	const double kSliderWeight = 2.3;
	BSlider* slider = new BSlider(BRect(0, 0, 1, 1), NULL, leftLabelContent,
		sliderMessage, sliderLowerBound, sliderUpperBound);
	groupLayout->AddView(slider, kSliderWeight);
	slider->ResizeToPreferred();
	slider->SetValue(sliderValue);
	slider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	slider->SetHashMarkCount(10);
	BString lowerBound, upperBound;
	lowerBound << sliderLowerBound;
	upperBound << sliderUpperBound;
	slider->SetLimitLabels(lowerBound.String(), upperBound.String());

	groupLayout->AddView(new BSeparatorView(B_VERTICAL));

	BTextView* valueText = _CreateTextBox();
	valueText->SetAlignment(B_ALIGN_CENTER);
	groupLayout->AddView(valueText);

	*sliderAttribute = slider;
	*valueTextAttribute = valueText;

	return groupLayout;
}


BTextView*
AlarmSettings::_CreateTextBox(const char* content)
{
	BTextView* text = new BTextView(static_cast<const char*>(NULL));
	text->SetText(content);
	text->MakeSelectable(false);
	text->MakeEditable(false);
	text->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	return text;
}


void
AlarmSettings::_ToggleAdditionalInput()
{
	bool enabled = fCheckBox->Value();
	fCriteriaSlider->SetEnabled(enabled);
	fRingsSlider->SetEnabled(enabled);
	fIntervalSlider->SetEnabled(enabled);
}


void
AlarmSettings::_UpdateShownValues()
{
	BString criteriaString;
	criteriaString << fCriteriaSlider->Value();
	fCriteriaValText->SetText(criteriaString.String());

	BString ringsString;
	ringsString << fRingsSlider->Value();
	fRingsValText->SetText(ringsString.String());

	BString intervalString;
	intervalString << fIntervalSlider->Value();
	fIntervalValText->SetText(intervalString.String());
}


void
AlarmSettings::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgToggleAlarm: 
			_ToggleAdditionalInput();
			fAlarm->FlipSwitch();
			break;
		case kMsgAlterAlarmCriteria: 
			_UpdateShownValues();
			fAlarm->SetCriteria(fCriteriaSlider->Value());
			break;
		case kMsgAlterAlarmRings: 
			_UpdateShownValues();
			fAlarm->SetRings(fRingsSlider->Value());
			break;
		case kMsgAlterAlarmInterval: 
			_UpdateShownValues();
			fAlarm->SetInterval(fIntervalSlider->Value());
			break;
		case kMsgActivateSoundsApp: 
			_ActivateExternalSounds();
			break;
		default:
			BWindow::MessageReceived(message);	    
	}
}


void
AlarmSettings::_ActivateExternalSounds()
{
	const int32 kArg_c = 1;
	const char* kArg_v[kArg_c + 1]; 
	const char* kEnviron[1];
	
	kArg_v[0] = BString("/boot/system/preferences/Sounds").String();
	kArg_v[1] = NULL;

	kEnviron[0] = NULL;

	resume_thread(load_image(kArg_c, kArg_v, kEnviron));
}

