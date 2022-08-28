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
#include <vector>

#include <InterfaceDefs.h>
#include <Message.h>
#include <OS.h>
#include <Roster.h>
#include <WindowInfo.h>

#include <tracker_private.h>

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
		case B_KEY_DOWN:
		{
			int32 key;
			if (message->FindInt32("key", &key) != B_OK)
				break;
			
			int32 modifiers;
			if (message->FindInt32("modifiers", &modifiers) != B_OK)
				break;
			
			int32 modifiersHeld = modifiers & (B_COMMAND_KEY
				| B_CONTROL_KEY | B_OPTION_KEY | B_MENU_KEY | B_SHIFT_KEY);
			
			if (modifiersHeld != (B_COMMAND_KEY | B_CONTROL_KEY) || key != 62)
				break;
			
			int32 tokenCount;
			int32* tokens;
			int32 token = -1;
			client_window_info* windowInfo;
			status_t status = BPrivate::get_window_order(current_workspace(),
				&tokens, &tokenCount);
			if (status != B_OK || !tokens || tokenCount < 1)
				break;

			// Run through all windows. If at least one is layer > 2 it means
			// it's being shown at the moment so we have to minimize it.
			std::vector<int32> minimizedWindowList;
			for (int i = 0; i < tokenCount; i++) {
				token = tokens[i];
				windowInfo = get_window_info(token);
				if (windowInfo->layer > 2) {
					// Find Deskbar and exclude it
					if (windowInfo->team != be_roster->TeamFor(kDeskbarSignature)) {
						do_window_action(token, B_MINIMIZE_WINDOW, BRect(), false);
						minimizedWindowList.push_back(token);
						snooze (10000); // sleep for 10ms
					}
				}
				free(windowInfo);
			}
			
			// If by the end of the loop minimizedWindowList is 0
			// then no windows were being shown so we will try to restore.
			if (minimizedWindowList.size() == 0) {
				int32 toRestore = fWindowsToRestore.size();
				for (int i = toRestore - 1; i >= 0; i--) {
					do_window_action(fWindowsToRestore[i], B_BRING_TO_FRONT, BRect(), false);
					snooze (10000); // sleep for 10ms
				}
				fWindowsToRestore.clear();
			// Otherwise, the windows we just minimized should be restored next time.
			} else {
				fWindowsToRestore = minimizedWindowList;
			}
			
			break;
		}
	}
	
	return B_DISPATCH_MESSAGE;
}


status_t
MinimizeAllInputFilter::InitCheck()
{
	return B_OK;
}
