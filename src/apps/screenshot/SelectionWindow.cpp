#include "SelectionWindow.h"

#include <Bitmap.h>
#include <Catalog.h>
#include <Cursor.h>
#include <Screen.h>
#include <String.h>

#include <stdio.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ScreenshotWindow"

const char *kInfoText1 = B_TRANSLATE("Left click and drag to select, press ENTER or Right Click to confirm.");
const char *kInfoText2 = B_TRANSLATE("Right click and drag to directly save the rect to the clipboard.");

const static rgb_color kSelectionColor = { 0, 111, 128, 100 };
const static rgb_color kInfoTextColor = { 233, 50, 50, 200 };

const static float kDraggerSize = 16;
const static float kDraggerSpacing = 2;
const static float kDraggerFullSize = kDraggerSize + kDraggerSpacing;

class SelectionView : public BView {
public:
	SelectionView(BRect frame, const char *name, const char* text = NULL);
	virtual void Draw(BRect updateRect);
	virtual BRect SelectionRect() const;

protected:
	bool fRightClickRect;
};


class SelectionViewRegion : public SelectionView {
public:
	enum drag_mode {
		DRAG_MODE_NONE = 0,
		DRAG_MODE_SELECT = 1,
		DRAG_MODE_MOVE = 2,
		DRAG_MODE_RESIZE_LEFT_TOP = 3,
		DRAG_MODE_RESIZE_RIGHT_TOP = 4,
		DRAG_MODE_RESIZE_LEFT_BOTTOM = 5,
		DRAG_MODE_RESIZE_RIGHT_BOTTOM = 6,
	};

	enum which_dragger {
		DRAGGER_NONE = -1,
		DRAGGER_LEFT_TOP = 0,
		DRAGGER_RIGHT_TOP = 1,
		DRAGGER_LEFT_BOTTOM = 2,
		DRAGGER_RIGHT_BOTTOM = 3
	};

	SelectionViewRegion(BRect frame, const char *name);
	virtual ~SelectionViewRegion();

	virtual void MouseDown(BPoint where);
	virtual void MouseUp(BPoint where);
	virtual void MouseMoved(BPoint where, uint32 code, const BMessage *message);
	virtual void KeyDown(const char* bytes, int32 numBytes);
	virtual void Draw(BRect updateRect);

	virtual BRect SelectionRect() const;

	BRect LeftTopDragger() const;
	BRect RightTopDragger() const;
	BRect LeftBottomDragger() const;
	BRect RightBottomDragger() const;

private:
	void _DrawDraggers();
	int _MouseOnDragger(BPoint where) const;

	BPoint fSelectionStart;
	BPoint fSelectionEnd;
	BPoint fCurrentMousePosition;
	BRect fStringRect;
	int fDragMode;
	BCursor* fCursorNWSE;
	BCursor* fCursorNESW;
	BCursor* fCursorGrab;
	BCursor* fCursorSelect;
};

// SelectionView
SelectionView::SelectionView(BRect frame, const char *name, const char* text)
	:
	BView(frame, name, B_FOLLOW_NONE, B_WILL_DRAW)
{
	SetFontSize(30);
}


void
SelectionView::Draw(BRect updateRect)
{
	PushState();
	SetDrawingMode(B_OP_COPY);
	SetHighColor(kInfoTextColor);

	float width_a = StringWidth(kInfoText1);
	float width_b = StringWidth(kInfoText2);
	BPoint point_a, point_b;
	point_a.x = (Bounds().Width() - width_a) / 2;
	point_a.y = Bounds().Height() / 2 -20;
	point_b.x = (Bounds().Width() - width_b) / 2;
	point_b.y = Bounds().Height() / 2 + 20;

	DrawString(kInfoText1, point_a);
	DrawString(kInfoText2, point_b);

	PopState();
}

BRect
SelectionView::SelectionRect() const
{
	return BRect(0, 0, -1, -1);
}


// SelectionViewRegion
SelectionViewRegion::SelectionViewRegion(BRect frame, const char *name)
	:
	SelectionView(frame, name),
	fSelectionStart(-1, -1),
	fSelectionEnd(-1, -1),
	fCursorNWSE(NULL),
	fCursorNESW(NULL),
	fCursorGrab(NULL),
	fCursorSelect(NULL)
{
	fDragMode = DRAG_MODE_NONE;
	fCursorNWSE = new BCursor(B_CURSOR_ID_RESIZE_NORTH_WEST_SOUTH_EAST);
	fCursorNESW = new BCursor(B_CURSOR_ID_RESIZE_NORTH_EAST_SOUTH_WEST);
	fCursorGrab = new BCursor(B_CURSOR_ID_GRABBING);
	fCursorSelect = new BCursor(B_CURSOR_ID_CROSS_HAIR);
}

SelectionViewRegion::~SelectionViewRegion()
{
	delete fCursorNWSE;
	delete fCursorNESW;
	delete fCursorGrab;
	delete fCursorSelect;
}


void
SelectionViewRegion::MouseDown(BPoint where)
{
	SelectionView::MouseDown(where);
	BPoint loc;
	uint32 btn;
	GetMouse(&loc, &btn);
	if(btn & B_SECONDARY_MOUSE_BUTTON) {
		SelectionView::fRightClickRect = true;
	}

	if (SelectionRect().Contains(where))
		fDragMode = DRAG_MODE_MOVE;
	else {
		switch (_MouseOnDragger(where)) {
			case DRAGGER_LEFT_TOP:
				fDragMode = DRAG_MODE_RESIZE_LEFT_TOP;
				break;
			case DRAGGER_RIGHT_TOP:
				fDragMode = DRAG_MODE_RESIZE_RIGHT_TOP;
				break;
			case DRAGGER_LEFT_BOTTOM:
				fDragMode = DRAG_MODE_RESIZE_LEFT_BOTTOM;
				break;
			case DRAGGER_RIGHT_BOTTOM:
				fDragMode = DRAG_MODE_RESIZE_RIGHT_BOTTOM;
				break;
			default:
				Invalidate();
				fDragMode = DRAG_MODE_SELECT;
				fSelectionStart = where;
				fSelectionEnd = where;
				break;
		}
	}
}


void
SelectionViewRegion::MouseMoved(BPoint where, uint32 code, const BMessage *message)
{
	if (fDragMode != DRAG_MODE_NONE) {
		BRect selectionRect = SelectionRect();
		float xOffset = where.x - fCurrentMousePosition.x;
		float yOffset = where.y - fCurrentMousePosition.y;
		switch (fDragMode) {
			case DRAG_MODE_SELECT:
			{
				SetViewCursor(fCursorSelect);
				fSelectionEnd = where;
				break;
			}
			case DRAG_MODE_MOVE:
			{
				SetViewCursor(fCursorGrab);
				fSelectionStart.x += xOffset;
				fSelectionStart.y += yOffset;
				fSelectionEnd.x += xOffset;
				fSelectionEnd.y += yOffset;
				break;
			}
			case DRAG_MODE_RESIZE_LEFT_TOP:
			{
				SetViewCursor(fCursorNWSE);
				fSelectionStart.x += xOffset;
				fSelectionStart.y += yOffset;
				break;
			}
			case DRAG_MODE_RESIZE_RIGHT_TOP:
			{
				SetViewCursor(fCursorNESW);
				fSelectionEnd.x += xOffset;
				fSelectionStart.y += yOffset;
				break;
			}
			case DRAG_MODE_RESIZE_LEFT_BOTTOM:
			{
				SetViewCursor(fCursorNESW);
				fSelectionStart.x += xOffset;
				fSelectionEnd.y += yOffset;
				break;
			}
			case DRAG_MODE_RESIZE_RIGHT_BOTTOM:
			{
				SetViewCursor(fCursorNWSE);
				fSelectionEnd.x += xOffset;
				fSelectionEnd.y += yOffset;
				break;
			}
			default:
				SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);
				break;
		}

		BRect newSelection = SelectionRect();
		BRect invalidateRect = (selectionRect | newSelection);
		invalidateRect.InsetBySelf(-(kDraggerSize + kDraggerSpacing), -(kDraggerSize + kDraggerSpacing));
		Invalidate(invalidateRect);
	} else
		SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);

	SelectionView::MouseMoved(where, code, message);

	fCurrentMousePosition = where;
}


void
SelectionViewRegion::MouseUp(BPoint where)
{
	SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);

	if (fDragMode == DRAG_MODE_SELECT)
		fSelectionEnd = where;

	fDragMode = DRAG_MODE_NONE;

	// Sanitize the selection rect (fSelectionStart < fSelectionEnd)
	BRect selectionRect = SelectionRect();
	fSelectionStart = selectionRect.LeftTop();
	fSelectionEnd = selectionRect.RightBottom();

	if(fRightClickRect == true) {
			Window()->PostMessage(B_QUIT_REQUESTED);
	}
	SelectionView::MouseUp(where);
}


void
SelectionViewRegion::KeyDown(const char* bytes, int32 numBytes)
{
	switch (bytes[0]) {
		case B_ENTER:
			Window()->PostMessage(B_QUIT_REQUESTED);
			break;
		case B_ESCAPE:
			fSelectionStart = BPoint(-1, -1);
			fSelectionEnd = BPoint(-1, -1);
			Window()->PostMessage(B_QUIT_REQUESTED);
			break;
		default:
			SelectionView::KeyDown(bytes, numBytes);
			break;
	}
}


void
SelectionViewRegion::Draw(BRect updateRect)
{
	SelectionView::Draw(updateRect);

	if (SelectionRect().IsValid()) {
		BRect selection = SelectionRect();
		SetDrawingMode(B_OP_ALPHA);
		SetHighColor(kSelectionColor);
		FillRect(selection);
		BString sizeString;
		sizeString << selection.IntegerWidth() << " x " << selection.IntegerHeight();
		float stringWidth = StringWidth(sizeString.String());
		BPoint position;
		position.x = (selection.Width() - stringWidth) / 2;
		position.y = selection.Height() / 2;
		position += selection.LeftTop();
		SetHighColor(255,255,255);
		SetLowColor(255,255,255);
		DrawString(sizeString.String(), position);
	}

	_DrawDraggers();
}


BRect
SelectionViewRegion::SelectionRect() const
{
	if (fSelectionStart == BPoint(-1, -1)
		&& fSelectionEnd == BPoint(-1, -1))
		return BRect(0, 0, -1, -1);

	BRect rect;
	rect.SetLeftTop(BPoint(min_c(fSelectionStart.x, fSelectionEnd.x),
					min_c(fSelectionStart.y, fSelectionEnd.y)));
	rect.SetRightBottom(BPoint(max_c(fSelectionStart.x, fSelectionEnd.x),
					max_c(fSelectionStart.y, fSelectionEnd.y)));
	return rect;
}


BRect
SelectionViewRegion::LeftTopDragger() const
{
	BPoint leftTop(min_c(fSelectionStart.x, fSelectionEnd.x),
					min_c(fSelectionStart.y, fSelectionEnd.y));
	return BRect(leftTop + BPoint(-kDraggerFullSize, -kDraggerFullSize),
			leftTop + BPoint(-kDraggerSpacing, -kDraggerSpacing));
}


BRect
SelectionViewRegion::RightTopDragger() const
{
	BPoint leftTop(max_c(fSelectionStart.x, fSelectionEnd.x),
					min_c(fSelectionStart.y, fSelectionEnd.y));
	return BRect(leftTop + BPoint(kDraggerSpacing, -kDraggerFullSize),
				leftTop + BPoint(kDraggerFullSize, -kDraggerSpacing));
}


BRect
SelectionViewRegion::LeftBottomDragger() const
{
	BPoint leftTop(min_c(fSelectionStart.x, fSelectionEnd.x),
					max_c(fSelectionStart.y, fSelectionEnd.y));
	return BRect(leftTop + BPoint(-kDraggerFullSize, kDraggerSpacing),
				leftTop + BPoint(-kDraggerSpacing, kDraggerFullSize));
}


BRect
SelectionViewRegion::RightBottomDragger() const
{
	BPoint leftTop(max_c(fSelectionStart.x, fSelectionEnd.x),
					max_c(fSelectionStart.y, fSelectionEnd.y));
	return BRect(leftTop + BPoint(kDraggerSpacing, kDraggerSpacing),
				leftTop + BPoint(kDraggerFullSize, kDraggerFullSize));
}


void
SelectionViewRegion::_DrawDraggers()
{
	if (fSelectionStart == BPoint(-1, -1)
		&& fSelectionEnd == BPoint(-1, -1))
		return;

	PushState();
	SetHighColor(0, 0, 0);
	StrokeRect(LeftTopDragger(), B_SOLID_HIGH);
	StrokeRect(LeftBottomDragger(), B_SOLID_HIGH);
	StrokeRect(RightTopDragger(), B_SOLID_HIGH);
	StrokeRect(RightBottomDragger(), B_SOLID_HIGH);
	PopState();
}


int
SelectionViewRegion::_MouseOnDragger(BPoint point) const
{
	if (LeftTopDragger().Contains(point))
		return DRAGGER_LEFT_TOP;
	else if (RightTopDragger().Contains(point))
		return DRAGGER_RIGHT_TOP;
	else if (LeftBottomDragger().Contains(point))
		return DRAGGER_LEFT_BOTTOM;
	else if (RightBottomDragger().Contains(point))
		return DRAGGER_RIGHT_BOTTOM;

	return DRAGGER_NONE;
}

// SelectionWindow
SelectionWindow::SelectionWindow(BMessenger& target, uint32 command)
	:
	BWindow(BScreen().Frame(), "Area Window", window_type(1026),
		B_ASYNCHRONOUS_CONTROLS|B_NOT_RESIZABLE|B_NOT_CLOSABLE|B_NOT_ZOOMABLE)
{
	fView = new SelectionViewRegion(Bounds(), "Selection view");
	AddChild(fView);
	fTarget  = target;
	fCommand = command;
}

void
SelectionWindow::Show()
{
	BBitmap *bitmap = NULL;
	BScreen(this).GetBitmap(&bitmap, false);
	fView->SetViewBitmap(bitmap);
	fView->MakeFocus(true);
	delete bitmap;

	BWindow::Show();
}

bool
SelectionWindow::QuitRequested()
{
	BScreen screen(this);
	BMessage message(fCommand);
	BBitmap *bitmap = NULL;
	BRect selection = fView->SelectionRect();
	if (!selection.IsValid())
		selection = screen.Frame();

	screen.GetBitmap(&bitmap, false, &selection);
	message.AddRect("selection", selection);
	message.AddPointer("bitmap", bitmap);

	fTarget.SendMessage(&message);

	return BWindow::QuitRequested();
}
