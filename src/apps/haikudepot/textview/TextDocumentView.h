/*
 * Copyright 2013-2015, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef TEXT_DOCUMENT_VIEW_H
#define TEXT_DOCUMENT_VIEW_H

#include <Invoker.h>
#include <String.h>
#include <View.h>

#include "TextDocument.h"
#include "TextDocumentLayout.h"
#include "TextEditor.h"


class BClipboard;
class BMessageRunner;


class TextDocumentView : public BView, public BInvoker {
public:
								TextDocumentView(const char* name = NULL);
	virtual						~TextDocumentView();

	// BView implementation
	virtual	void				MessageReceived(BMessage* message);

	virtual void				Draw(BRect updateRect);

	virtual	void				AttachedToWindow();
	virtual void				FrameResized(float width, float height);
	virtual	void				WindowActivated(bool active);
	virtual	void				MakeFocus(bool focus = true);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);

	virtual	void				KeyDown(const char* bytes, int32 numBytes);
	virtual	void				KeyUp(const char* bytes, int32 numBytes);

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();

	virtual	bool				HasHeightForWidth();
	virtual	void				GetHeightForWidth(float width, float* min,
									float* max, float* preferred);

	virtual void				Relayout();

	// TextDocumentView interface
			void				SetTextDocument(
									const TextDocumentRef& document);

			void				SetEditingEnabled(bool enabled);
			void				SetTextEditor(
									const TextEditorRef& editor);

			void				SetInsets(float inset);
			void				SetInsets(float horizontal, float vertical);
			void				SetInsets(float left, float top, float right,
									float bottom);

			void				SetSelectionEnabled(bool enabled);
			void				SetCaret(BPoint where, bool extendSelection);

			void				SelectAll();
			bool				HasSelection() const;
			void				GetSelection(int32& start, int32& end) const;

			void				Copy(BClipboard* clipboard);
			void				Paste(BClipboard* clipboard);

private:
			float				_TextLayoutWidth(float viewWidth) const;

			void				_UpdateScrollBars();

			void				_ShowCaret(bool show);
			void				_BlinkCaret();
			void				_DrawCaret(int32 textOffset);
			void				_DrawSelection();
			void				_GetSelectionShape(BShape& shape,
									int32 start, int32 end);

			status_t			_PastePossiblyDisallowedChars(const char* str, int32 maxLength);
			void				_PasteAllowedChars(const char* str, int32 maxLength);
	static	bool				_IsAllowedChar(char c);
	static	bool				_AreCharsAllowed(const char* str, int32 maxLength);

private:
			TextDocumentRef		fTextDocument;
			TextDocumentLayout	fTextDocumentLayout;
			TextEditorRef		fTextEditor;

			float				fInsetLeft;
			float				fInsetTop;
			float				fInsetRight;
			float				fInsetBottom;

			BRect				fCaretBounds;
			BMessageRunner*		fCaretBlinker;
			int32				fCaretBlinkToken;
			bool				fSelectionEnabled;
			bool				fShowCaret;
};

#endif // TEXT_DOCUMENT_VIEW_H
