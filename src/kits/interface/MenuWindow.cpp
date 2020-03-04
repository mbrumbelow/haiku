/*
 * Copyright 2001-2020 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini, stefano.ceccherini@gmail.com
 *		Marc Flerackers, mflerackers@androme.be
 *		John Scipione, jscipione@gmail.com
 */

//!	BMenuWindow is a custom BWindow for BMenus.

#include <MenuWindow.h>

#include <ControlLook.h>
#include <Debug.h>
#include <GroupLayout.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Point.h>
#include <Screen.h>

#include <MenuPrivate.h>
#include <WindowPrivate.h>


namespace BPrivate {

class BMenuScroller : public BControl {
public:
							BMenuScroller(BRect frame);
};


class BMenuFrame : public BView {
public:
							BMenuFrame(BMenu* menu);

	virtual	void			AttachedToWindow();
	virtual	void			DetachedFromWindow();
	virtual	void			Draw(BRect updateRect);
	virtual	void			LayoutChanged();

private:
	friend class BMenuWindow;

			BMenu*			fMenu;
};


class UpperScroller : public BMenuScroller {
public:
							UpperScroller(BRect frame);

	virtual	void			Draw(BRect updateRect);
};


class LowerScroller : public BMenuScroller {
public:
							LowerScroller(BRect frame);

	virtual	void			Draw(BRect updateRect);
};


void
moveSubmenusOver(BMenu* menu, BRect menuFrame, BRect screenFrame)
{
	if (menu == NULL)
		return;

	for (int32 i = menu->CountItems(); i-- > 0;) {
		BMenu* submenu = menu->SubmenuAt(i);
		if (submenu == NULL || submenu->Window() == NULL)
			continue; // not an open submenu, next

		BMenuWindow* submenuWindow
			= dynamic_cast<BMenuWindow*>(submenu->Window());
		if (submenuWindow == NULL)
			break; // submenu window was not a BMenuWindow, strange if true

		// found an open submenu, get submenu frame
		BPoint submenuLoc;
		BRect submenuFrame;
		if (submenu->LockLooper()) {
			// need to lock looper because we're in a different thread
			submenuFrame = submenu->Frame();
			submenu->ConvertToScreen(&submenuFrame);
			submenu->UnlockLooper();
		} else
			break; // give up

		// get submenu loc and convert it to screen coords using menu
		if (menu->LockLooper()) {
			// check if submenu should be displayed right or left of menu
			submenuLoc = (submenuFrame.right < menuFrame.right
				? submenu->Superitem()->Frame().LeftTop()
					- BPoint(submenuFrame.Width() + 1, -1)
				: submenu->Superitem()->Frame().RightTop() + BPoint(1, 1));
			menu->ConvertToScreen(&submenuLoc);
			submenuFrame.OffsetTo(submenuLoc);
			menu->UnlockLooper();
		} else
			break; // give up

		// move submenu frame into screen bounds vertically
		if (submenuFrame.Height() < screenFrame.Height()) {
			if (submenuFrame.bottom >= screenFrame.bottom)
				submenuLoc.y -= (submenuFrame.bottom - screenFrame.bottom);
			else if (submenuFrame.top <= screenFrame.top)
				submenuLoc.y += (screenFrame.top - submenuFrame.top);
		} else {
			// put menu at top of screen, turn on the scroll arrows
			submenuLoc.y = 0;
		}

		// move submenu window into place
		submenuWindow->MoveTo(submenuLoc);

		// recurse through submenu's submenus
		moveSubmenusOver(submenu, submenuFrame, screenFrame);

		// we're done with this menu
		break;
	}
}

}	// namespace BPrivate


using namespace BPrivate;


const float kScrollerHeight = 12.f;
const float kScrollStep = 19.f;


BMenuScroller::BMenuScroller(BRect frame)
	:
	BControl(frame, "menu scroller", "", NULL, 0,
		B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE)
{
	SetViewUIColor(B_MENU_BACKGROUND_COLOR);
}


//	#pragma mark -


UpperScroller::UpperScroller(BRect frame)
	:
	BMenuScroller(frame)
{
}


void
UpperScroller::Draw(BRect updateRect)
{
	SetLowColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_1_TINT));
	float middle = Bounds().right / 2;

	// Draw the upper arrow.
	if (IsEnabled())
		SetHighColor(0, 0, 0);
	else {
		SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
			B_DARKEN_2_TINT));
	}

	FillRect(Bounds(), B_SOLID_LOW);

	FillTriangle(BPoint(middle, (kScrollerHeight / 2) - 3),
		BPoint(middle + 5, (kScrollerHeight / 2) + 2),
		BPoint(middle - 5, (kScrollerHeight / 2) + 2));
}


//	#pragma mark -


LowerScroller::LowerScroller(BRect frame)
	:
	BMenuScroller(frame)
{
}


void
LowerScroller::Draw(BRect updateRect)
{
	SetLowColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_1_TINT));

	BRect frame = Bounds();
	// Draw the lower arrow.
	if (IsEnabled())
		SetHighColor(0, 0, 0);
	else {
		SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
			B_DARKEN_2_TINT));
	}

	FillRect(frame, B_SOLID_LOW);

	float middle = Bounds().right / 2;

	FillTriangle(BPoint(middle, frame.bottom - (kScrollerHeight / 2) + 3),
		BPoint(middle + 5, frame.bottom - (kScrollerHeight / 2) - 2),
		BPoint(middle - 5, frame.bottom - (kScrollerHeight / 2) - 2));
}


//	#pragma mark -


BMenuFrame::BMenuFrame(BMenu* menu)
	:
	BView("menu frame", B_WILL_DRAW),
	fMenu(menu)
{
}


void
BMenuFrame::AttachedToWindow()
{
	BView::AttachedToWindow();

	if (fMenu != NULL)
		AddChild(fMenu);

	ResizeTo(Window()->Bounds().Width(), Window()->Bounds().Height());
	if (fMenu != NULL) {
		BFont font;
		fMenu->GetFont(&font);
		SetFont(&font);
	}
}


void
BMenuFrame::DetachedFromWindow()
{
	if (fMenu != NULL)
		RemoveChild(fMenu);
}


void
BMenuFrame::Draw(BRect updateRect)
{
	if (fMenu != NULL && fMenu->CountItems() == 0) {
		BRect rect(Bounds());
		be_control_look->DrawMenuBackground(this, rect, updateRect,
			ui_color(B_MENU_BACKGROUND_COLOR));
		SetDrawingMode(B_OP_OVER);

		// TODO: Review this as it's a bit hacky.
		// Since there are no items in this menu, its size is 0x0.
		// To show an empty BMenu, we use BMenuFrame to draw an empty item.
		// It would be nice to simply add a real "empty" item, but in that case
		// we couldn't tell if the item was added by us or not, and applications
		// could break (because CountItems() would return 1 for an empty BMenu).
		// See also BMenu::UpdateWindowViewSize()
		font_height height;
		GetFontHeight(&height);
		SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
			B_DISABLED_LABEL_TINT));
		BPoint where(
			(Bounds().Width() - fMenu->StringWidth(kEmptyMenuLabel)) / 2,
			ceilf(height.ascent + 1));
		DrawString(kEmptyMenuLabel, where);
	}
}


void
BMenuFrame::LayoutChanged()
{
	if (fMenu == NULL || Window() == NULL)
		return BView::LayoutChanged();

	// resize window to menu width
	Window()->ResizeTo(fMenu->Frame().Width(), Window()->Frame().Height());

	// push child menus over recursively
	moveSubmenusOver(fMenu, fMenu->ConvertToScreen(fMenu->Frame()),
		(BScreen(fMenu->Window())).Frame());

	BView::LayoutChanged();
}



//	#pragma mark -


BMenuWindow::BMenuWindow(const char* name)
	// The window will be resized by BMenu, so just pass a dummy rect
	:
	BWindow(BRect(0, 0, 0, 0), name, B_BORDERED_WINDOW_LOOK, kMenuWindowFeel,
		B_NOT_MOVABLE | B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_AVOID_FOCUS
			| kAcceptKeyboardFocusFlag),
	fMenu(NULL),
	fMenuFrame(NULL),
	fUpperScroller(NULL),
	fLowerScroller(NULL),
	fScrollStep(kScrollStep)
{
	SetSizeLimits(2, B_SIZE_UNLIMITED, 2, B_SIZE_UNLIMITED);
	SetLayout(new BGroupLayout(B_VERTICAL, 0));
}


BMenuWindow::~BMenuWindow()
{
	DetachMenu();
}


void
BMenuWindow::DispatchMessage(BMessage *message, BHandler *handler)
{
	BWindow::DispatchMessage(message, handler);
}


void
BMenuWindow::AttachMenu(BMenu* menu)
{
	if (fMenuFrame != NULL)
		debugger("BMenuWindow: a menu is already attached!");

	if (menu != NULL) {
		fMenuFrame = new BMenuFrame(menu);
		GetLayout()->AddView(1, fMenuFrame);
		menu->MakeFocus(true);
		fMenu = menu;
	}
}


void
BMenuWindow::DetachMenu()
{
	DetachScrollers();
	if (fMenuFrame != NULL) {
		GetLayout()->RemoveView(fMenuFrame);
		delete fMenuFrame;
		fMenuFrame = NULL;
		fMenu = NULL;
	}
}


void
BMenuWindow::AttachScrollers()
{
	// We want to attach a scroller only if there's a
	// menu frame already existing.
	if (fMenu == NULL || fMenuFrame == NULL)
		return;

	fMenu->MakeFocus(true);

	BRect frame = Bounds();
	float newLimit = fMenu->Bounds().Height()
		- (frame.Height() - 2 * kScrollerHeight);

	if (!HasScrollers())
		fValue = 0;
	else if (fValue > newLimit)
		_ScrollBy(newLimit - fValue);

	fLimit = newLimit;

	if (fUpperScroller == NULL) {
		fUpperScroller = new UpperScroller(
			BRect(0, 0, frame.right, kScrollerHeight - 1));
		GetLayout()->AddView(0, fUpperScroller);
	}

	if (fLowerScroller == NULL) {
		fLowerScroller = new LowerScroller(
			BRect(0, frame.bottom - kScrollerHeight + 1, frame.right,
				frame.bottom));
		GetLayout()->AddView(2, fLowerScroller);
	}

	fUpperScroller->ResizeTo(frame.right, kScrollerHeight - 1);
	fLowerScroller->ResizeTo(frame.right, kScrollerHeight - 1);

	fUpperScroller->SetEnabled(fValue > 0);
	fLowerScroller->SetEnabled(fValue < fLimit);

	fMenuFrame->ResizeTo(frame.Width(), frame.Height() - 2 * kScrollerHeight);
	fMenuFrame->MoveTo(0, kScrollerHeight);
}


void
BMenuWindow::DetachScrollers()
{
	// BeOS doesn't remember the position where the last scrolling ended,
	// so we just scroll back to the beginning.
	if (fMenu != NULL)
		fMenu->ScrollTo(0, 0);

	if (fLowerScroller != NULL) {
		GetLayout()->RemoveView(fLowerScroller);
		delete fLowerScroller;
		fLowerScroller = NULL;
	}

	if (fUpperScroller != NULL) {
		GetLayout()->RemoveView(fUpperScroller);
		delete fUpperScroller;
		fUpperScroller = NULL;
	}

	BRect frame = Bounds();

	if (fMenuFrame != NULL) {
		fMenuFrame->ResizeTo(frame.Width(), frame.Height());
		fMenuFrame->MoveTo(0, 0);
	}
}


void
BMenuWindow::SetSmallStep(float step)
{
	fScrollStep = step;
}


void
BMenuWindow::GetSteps(float* _smallStep, float* _largeStep) const
{
	if (_smallStep != NULL)
		*_smallStep = fScrollStep;
	if (_largeStep != NULL) {
		if (fMenuFrame != NULL)
			*_largeStep = fMenuFrame->Bounds().Height() - fScrollStep;
		else
			*_largeStep = fScrollStep * 2;
	}
}


bool
BMenuWindow::HasScrollers() const
{
	return fMenuFrame != NULL && fUpperScroller != NULL
		&& fLowerScroller != NULL;
}


bool
BMenuWindow::CheckForScrolling(const BPoint &cursor)
{
	if (!fMenuFrame || !fUpperScroller || !fLowerScroller)
		return false;

	return _Scroll(cursor);
}


bool
BMenuWindow::TryScrollBy(const float& step)
{
	if (!fMenuFrame || !fUpperScroller || !fLowerScroller)
		return false;

	_ScrollBy(step);
	return true;
}


bool
BMenuWindow::TryScrollTo(const float& where)
{
	if (!fMenuFrame || !fUpperScroller || !fLowerScroller)
		return false;

	_ScrollBy(where - fValue);
	return true;
}


bool
BMenuWindow::_Scroll(const BPoint& where)
{
	ASSERT((fLowerScroller != NULL));
	ASSERT((fUpperScroller != NULL));

	const BPoint cursor = ConvertFromScreen(where);
	const BRect &lowerFrame = fLowerScroller->Frame();
	const BRect &upperFrame = fUpperScroller->Frame();

	int32 delta = 0;
	if (fLowerScroller->IsEnabled() && lowerFrame.Contains(cursor))
		delta = 1;
	else if (fUpperScroller->IsEnabled() && upperFrame.Contains(cursor))
		delta = -1;

	if (delta == 0)
		return false;

	float smallStep;
	GetSteps(&smallStep, NULL);
	_ScrollBy(smallStep * delta);

	snooze(5000);

	return true;
}


void
BMenuWindow::_ScrollBy(const float& step)
{
	BRect frame(Bounds());

	if (step > 0) {
		if (fValue == 0) {
			fUpperScroller->SetEnabled(true);
			fUpperScroller->Invalidate();
		}

		if (fValue + step >= fLimit) {
			// If we reached the limit, only scroll to the end
			fMenu->ScrollBy(0, fLimit - fValue);
			fValue = fLimit;
			fLowerScroller->SetEnabled(false);
			fLowerScroller->Invalidate();
		} else {
			fMenu->ScrollBy(0, step);
			fValue += step;
		}

		fMenuFrame->Invalidate(BRect(frame.left, frame.top,
			frame.right, frame.top + (kScrollerHeight + 1) + step * 2));
	} else if (step < 0) {
		if (fValue == fLimit) {
			fLowerScroller->SetEnabled(true);
			fLowerScroller->Invalidate();
		}

		if (fValue + step <= 0) {
			fMenu->ScrollBy(0, -fValue);
			fValue = 0;
			fUpperScroller->SetEnabled(false);
			fUpperScroller->Invalidate();
		} else {
			fMenu->ScrollBy(0, step);
			fValue += step;
		}

		fMenuFrame->Invalidate(BRect(frame.left,
			frame.bottom - (kScrollerHeight + 1) - step * 2,
			frame.right, frame.bottom));
	}
}

