/*
 * Copyright 2015 Haiku, Inc. All rights reserved
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */


#include "MinimizeAllInputFilter.h"

#include <string.h>

#include <new>

#include <InterfaceDefs.h>
#include <Message.h>
#include <OS.h>
#include <Roster.h>
#include <WindowInfo.h>

#include <tracker_private.h>
#include <syslog.h>

#define _MINIMIZE_ALL_		'_WMA'


extern "C" BInputServerFilter* instantiate_input_filter() {
	return new(std::nothrow) MinimizeAllInputFilter();
}


MinimizeAllInputFilter::MinimizeAllInputFilter()
{
}


filter_result
MinimizeAllInputFilter::Filter(BMessage* message, BList* _list)
{
	switch (message->what) {
		// Reset to "minimize all" behaviour as soon as an app is activated.
		//case B_SOME_APP_ACTIVATED:
		//	fMinimizeAll = true;
		//	break;
			
		case B_KEY_DOWN:
		{
			syslog(LOG_ERR, "%s\n", fMinimizeAll ? "true" : "false");
			int32 key;
			if (message->FindInt32("key", &key) != B_OK)
				break;
			
			int32 modifiers;
			if (message->FindInt32("modifiers", &modifiers) != B_OK)
				break;
			
			int32 modifiersHeld = modifiers & (B_COMMAND_KEY
				| B_CONTROL_KEY | B_OPTION_KEY | B_MENU_KEY | B_SHIFT_KEY);
			
			if (modifiersHeld != B_OPTION_KEY || key != 62)
				break;
							
			int32 cookie = 0;
			team_info teamInfo;
			while (get_next_team_info(&cookie, &teamInfo) == B_OK) {
				app_info appInfo;
				be_roster->GetRunningAppInfo(teamInfo.team, &appInfo);
				team_id team = appInfo.team;
				be_roster->ActivateApp(team);

				if (be_roster->GetActiveAppInfo(&appInfo) == B_OK
					&& (appInfo.flags & B_BACKGROUND_APP) == 0
					&& strcasecmp(appInfo.signature, kDeskbarSignature) != 0) {
					BRect zoomRect;
					if (fMinimizeAll)
						do_minimize_team(zoomRect, team, false);
					else
						do_bring_to_front_team(zoomRect, team, false);
				}
			}
			fMinimizeAll = !fMinimizeAll;
			break;
		}
	}
	
	return B_DISPATCH_MESSAGE;
}


status_t
MinimizeAllInputFilter::InitCheck()
{
	fMinimizeAll = true;
	return B_OK;
}
