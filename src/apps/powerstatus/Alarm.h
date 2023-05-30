#ifndef ALARM_H
#define ALARM_H

#include <OS.h>

class Alarm {
public:
							Alarm(bool activate = false,
			 					int criteria = 100,
								int rings = 5,
								int interval = 5);
							~Alarm();

			void 			SetActivation(bool activate);
			void 			FlipSwitch();
			bool 			IsActivated();

			void 			SetCriteria(int percentage);
			int 			Criteria();

			void 			SetRings(int rings);
			int 			Rings();
			
			void 			SetInterval(int interval);
			int 			Interval();

			sem_id 			Sem();

			bool 			CriteriaMet(int prevPercent, int currentPercent);

			void 			Ring();
			
	static 	int32 			Activate(void* alarm);
private:

			thread_id 		fThread;
			sem_id 			fSem;
			
			bool 			fActivated;
			int 			fCriteria;
			int 			fRings;
			int 			fInterval;
				// in milliseconds
};

#endif
