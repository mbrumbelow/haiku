#ifndef ALARMWINDOW_H
#define ALARMWINDOW_H


#include <Window.h>


const uint32 kMsgToggleAlarm = 'tgal';
const uint32 kMsgAlterAlarmCriteria = 'alcr';
const uint32 kMsgAlterAlarmRings = 'alar';
const uint32 kMsgAlterAlarmInterval = 'alai';
const uint32 kMsgActivateSoundsApp = 'acsa';

const uint32 kMsgAlarmSettingsRequested = 'alrq';


class Alarm;
class BMessage;
class BTextView;
class BGroupLayout;
class BSlider;
class BCheckBox;


class AlarmSettings : public BWindow {
public:
							AlarmSettings(Alarm* alarm);
							~AlarmSettings();

private:
			void			MessageReceived(BMessage* message);	
			void			_ToggleAdditionalInput();
			void			_UpdateShownValues();
			void			_ActivateExternalSounds();

			BGroupLayout*	_MakeEnablerSoundsGroup(BGroupLayout* parent);
			BGroupLayout*	_MakeCriteriaGroup(BGroupLayout* parent);
			BGroupLayout*	_MakeRingsGroup(BGroupLayout* parent);
			BGroupLayout*	_MakeIntervalGroup(BGroupLayout* parent);

			BTextView*		_CreateTextBox(const char* content = NULL);

			BGroupLayout*	_CreateIntegerInputComponent(
								BGroupLayout* parent,
								const char* leftLabelContent,
								BMessage* sliderMessage,
								int sliderUpperBound,
								int sliderLowerBound,
								int sliderValue,
								BSlider** sliderAttribute,
								BTextView** valueTextAttribute);

private:
			Alarm*			fAlarm;

			// various child components of the AlarmSettings Window for easy reference
			BCheckBox* 		fCheckBox;
			BSlider*		fCriteriaSlider;

			BTextView*		fCriteriaValText;
			BSlider*		fRingsSlider;

			BTextView*		fRingsValText;
			BSlider*		fIntervalSlider;

			BTextView*		fIntervalValText;
};

#endif

