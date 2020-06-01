/*
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef TAB_VIEW_H
#define TAB_VIEW_H

#include <AbstractLayoutItem.h>
#include <Rect.h>
#include <String.h>


class BMessage;
class BView;
class TabContainerView;
class TabLayoutItem;


class TabView {
public:
								TabView();
	virtual						~TabView();

	virtual	BSize				MinSize();
	virtual	BSize				PreferredSize();
	virtual	BSize				MaxSize();

			void 				Draw(BRect updateRect);
	virtual	void				DrawBackground(BView* owner, BRect frame,
									const BRect& updateRect, bool isFirst,
									bool isLast, bool isFront, bool isFull);
	virtual	void				DrawContents(BView* owner, BRect frame,
									const BRect& updateRect);

	virtual	void				MouseDown(BPoint where, uint32 buttons);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);

			void				SetIsFirst(bool isFirst);
			bool				IsFirst() const { return fIsFirst; };

			void				SetIsLast(bool isLast);
			bool				IsLast() const { return fIsLast; };

			void				SetIsFront(bool isFront);
			bool				IsFront() const { return fIsFront; };

			void				SetIsFull(bool isFull);
			bool				IsFull() const { return fIsFull; };

	virtual	void				Update(bool isFirst, bool isLast,
									bool isFront, bool isFull);

			BLayoutItem*		LayoutItem() const;
			void				SetContainerView(
									TabContainerView* containerView);
			TabContainerView*	ContainerView() const;

			void				SetLabel(const char* label);
			const BString&		Label() const;

			BRect				Frame() const;

private:
			float				_LabelHeight() const;

private:
			TabContainerView*	fContainerView;
			TabLayoutItem*		fLayoutItem;

			BString				fLabel;

			bool				fIsFirst : 1;
			bool				fIsLast : 1;
			bool				fIsFront : 1;
			bool				fIsFull : 1;
};


class TabLayoutItem : public BAbstractLayoutItem {
public:
								TabLayoutItem(TabView* parent);

	virtual	bool				IsVisible();
	virtual	void				SetVisible(bool visible);

	virtual	BRect				Frame();
	virtual	void				SetFrame(BRect frame);

	virtual	BView*				View();

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

			TabView*			Parent() const;
			void				InvalidateContainer();
			void				InvalidateContainer(BRect frame);
private:
			TabView*			fParent;
			BRect				fFrame;
			bool				fVisible;
};



#endif // TAB_VIEW_H
