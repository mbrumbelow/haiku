/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/


#include "SlowMenu.h"

#include <PopUpMenu.h>


const int32 kItemsToAddChunk = 20;
const bigtime_t kMaxTimeBuildingMenu = 200000;


//	#pragma mark - BSlowMenu


template<typename BaseMenu>
BSlowMenu<BaseMenu>::BSlowMenu(const char* title, menu_layout layout)
	:
	BaseMenu(title, layout),
	fMenuBuilt(false)
{
}


template<typename BaseMenu>
bool
BSlowMenu<BaseMenu>::AddDynamicItem(BMenu::add_state state)
{
	if (fMenuBuilt)
		return false;

	if (state == BMenu::B_ABORT) {
		ClearMenuBuildingState();
		return false;
	}

	if (state == BMenu::B_INITIAL_ADD && !StartBuildingItemList()) {
		ClearMenuBuildingState();
		return false;
	}

	bigtime_t timeToBail = system_time() + kMaxTimeBuildingMenu;
	for (int32 count = 0; count < kItemsToAddChunk; count++) {
		if (!AddNextItem()) {
			fMenuBuilt = true;
			DoneBuildingItemList();
			ClearMenuBuildingState();
			return false;
				// done with menu, don't call again
		}

		if (system_time() > timeToBail) {
			// we've been in here long enough, come back later
			break;
		}
	}

	return true;
		// call me again, got more to show
}


template<typename BaseMenu>
bool
BSlowMenu<BaseMenu>::StartBuildingItemList()
{
	return true;
}


//	#pragma mark - template instantiation


namespace BPrivate {
template class BSlowMenu<BMenu>;
template class BSlowMenu<BPopUpMenu>;
}
