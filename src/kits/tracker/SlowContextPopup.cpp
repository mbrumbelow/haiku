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

#include <string.h>
#include <stdlib.h>

#include "SlowContextPopup.h"


//	#pragma mark - BSlowContextMenu


BSlowContextMenu::BSlowContextMenu(const char* title)
	:
	_inherited(title, B_REFS_RECEIVED, BMessenger(), NULL, NULL),
	fIsShowing(false)
{
	SetFont(be_plain_font);
}


BSlowContextMenu::~BSlowContextMenu()
{
}


void
BSlowContextMenu::AttachedToWindow()
{
	// showing flag is set immediately as
	// it may take a while to build the menu's
	// contents.
	//
	// it should get set only once when Go is called
	// and will get reset in DetachedFromWindow
	//
	// this flag is used in ContainerWindow::ShowContextMenu
	// to determine whether we should show this menu, and
	// the only reason we need to do this is because this
	// menu is spawned ::Go as an asynchronous menu, which
	// is done because we will deadlock if the target's
	// window is open...  so there
	fIsShowing = true;

	_inherited::AttachedToWindow();
}


void
BSlowContextMenu::DetachedFromWindow()
{
	// see note above in AttachedToWindow
	fIsShowing = false;

	_inherited::DetachedFromWindow();
}


void
BSlowContextMenu::ClearMenu()
{
	RemoveItems(0, CountItems(), true);

	fMenuBuilt = false;
}
