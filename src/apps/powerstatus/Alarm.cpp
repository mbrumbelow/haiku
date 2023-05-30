#include "Alarm.h"

#include <SupportDefs.h>
#include <OS.h>
#include <Beep.h>

#include <stdio.h>

Alarm::Alarm(bool activate, int criteria, int rings, int interval)
	:
	fThread(spawn_thread(&Alarm::Activate, "alarm thread", B_URGENT_PRIORITY, this)),
		// another thread will be created to specially ring alarms
	fSem(create_sem(0, "alarm sem")),
	// initialized variables below are set by the user in the AlarmSettings window
	fActivated(activate),
		// fActivated represents whether the alarm is enabled
	fCriteria(criteria),
		// fCriteria represents the battery percentage that when reached,
		// will ring the alarm
	fRings(rings),
		// fRings represents how many times the audio file, specified by the user
		// as the "Battery alarm" event in Sounds, will be played when the alarm is activated
	fInterval(interval)
		// fInterval represents the duration of the interval between played repeats of 
		// the audio file
{
	add_system_beep_event("Battery alarm");
}

Alarm::~Alarm()
{
	kill_thread(fThread);
	delete_sem(fSem);
}

bool
Alarm::CriteriaMet(int prevPercent, int currentPercent)
{
	return (prevPercent > fCriteria && currentPercent <= fCriteria) ||
		(prevPercent < fCriteria && currentPercent >= fCriteria);
}

bool
Alarm::IsActivated()
{
	return fActivated;
}

void
Alarm::SetActivation(bool activate)
{
	fActivated = activate;
}

void
Alarm::FlipSwitch()
{
	fActivated = !fActivated;
}

int
Alarm::Criteria()
{
	return fCriteria;
}

void
Alarm::SetCriteria(int percentage)
{
	fCriteria = percentage;
}

void
Alarm::Ring()
{
	resume_thread(fThread);
}

sem_id
Alarm::Sem()
{
	return fSem;
}

void
Alarm::SetRings(int rings)
{
	fRings = rings;
}

int
Alarm::Rings()
{
	return fRings;
}

void
Alarm::SetInterval(int interval)
{
	fInterval = interval;
}

int
Alarm::Interval()
{
	return fInterval;
}

int32
Alarm::Activate(void* vAlarm)
{
	Alarm* alarm = static_cast<Alarm*>(vAlarm);

	while (true) {
		for (int32 i = 0; i < alarm->Rings(); i++) {
			system_beep("Battery alarm");
			acquire_sem_etc(alarm->Sem(), 1, B_RELATIVE_TIMEOUT,
				alarm->Interval() * 100000);
					// alarm->interval(), which is in milliseconds,
					// is multiplied further because acquire_sem_etc
					// takes in its timeout argument in microseconds 
		}
		suspend_thread(find_thread(NULL));
	}

	return B_OK;
}
