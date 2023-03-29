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

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered
trademarks of Be Incorporated in the United States and other countries. Other
brand product names are registered trademarks or trademarks of their respective
holders.
All rights reserved.
*/
#ifndef SWITCH_UTILS_H
#define SWITCH_UTILS_H


#include <InterfaceDefs.h>
#include <WindowInfo.h>

#include "BarApp.h"


inline int32
LowBitIndex(uint32 value)
{
	int32 result = 0;
	int32 bitMask = 1;

	if (value == 0)
		return -1;

	while (result < 32 && (value & bitMask) == 0) {
		result++;
		bitMask = bitMask << 1;
	}

	return result;
}


inline bool
IsVisibleInCurrentWorkspace(const window_info* windowInfo)
{
	// The window list is always ordered from the top front visible window
	// (the first on the list), going down through all the other visible
	// windows, then all hidden or non-workspace visible windows at the end.
	//     layer > 2  : normal visible window
	//     layer == 2 : reserved for the desktop window (visible also)
	//     layer < 2  : hidden (0) and non workspace visible window (1)
	return windowInfo->layer > 2;
}


inline bool
IsKeyDown(int32 key)
{
	key_info keyInfo;

	get_key_info(&keyInfo);
	return (keyInfo.key_states[key >> 3] & (1 << ((7 - key) & 7))) != 0;
}


inline bool
IsWindowOK(const window_info* window)
{
	// is_mini (true means that the window is minimized).
	// if not, then show_hide >= 1 means that the window is hidden.
	// If the window is both minimized and hidden, then you get :
	//     TWindow->is_mini = false;
	//     TWindow->was_mini = true;
	//     TWindow->show_hide >= 1;

	if (window->feel != _STD_W_TYPE_)
		return false;

	if (window->is_mini)
		return true;

	return window->show_hide_level <= 0;
}


inline void
BringToFront(const window_info* window)
{
	do_window_action(window->server_token, B_BRING_TO_FRONT,
		BRect(0, 0, 0, 0), false);
}


inline int
SmartStrcmp(const char* s1, const char* s2)
{
	if (strcasecmp(s1, s2) == 0)
		return 0;

	// if the strings on differ in spaces or underscores they still match
	while (*s1 && *s2) {
		if ((*s1 == ' ') || (*s1 == '_')) {
			s1++;
			continue;
		}
		if ((*s2 == ' ') || (*s2 == '_')) {
			s2++;
			continue;
		}
		if (*s1 != *s2) {
			// they differ
			return 1;
		}
		s1++;
		s2++;
	}

	// if one of the strings ended before the other
	// TODO: could process trailing spaces and underscores
	if (*s1)
		return 1;
	if (*s2)
		return 1;

	return 0;
}


#endif	// SWITCH_UTILS_H
