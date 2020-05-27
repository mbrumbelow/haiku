/*
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef LAST_DONE_STORE_H
#define LAST_DONE_STORE_H


#include <Locker.h>
#include <Path.h>
#include <String.h>


class LastDone;
class LastDones;


/*!	The idea with this class is that it keeps a persisted record of when tasks
	where last performed.  Some tasks may only need to happen every X days and
	this mechanism allows the system to keep track (between executions of the
	application) of when the task was last performed to establish if it is
	time to do the task again.
*/

class LastDoneStore {
public:
								LastDoneStore();
	virtual						~LastDoneStore();

			status_t			Init(const BPath& path);

			uint64				DaysSinceLastDone(const BString& type,
									const BString& key);
			status_t			MarkLastDone(const BString& type,
 									const BString& key);

private:
			uint64				_LastDone(const BString& type,
									const BString& key) const;
			LastDone*			_FindOrCreateLastDone(const BString& type,
									const BString& key) const;
			LastDone*			_FindLastDone(const BString& type,
									const BString& key) const;
			status_t			_Store() const;

private:
			BLocker				fLock;
			LastDones*			fLastDones;
			BPath				fPath;
};


#endif // LAST_DONE_STORE_H