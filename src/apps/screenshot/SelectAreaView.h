/*
 * Copyright 2025, Haiku, Inc.
 * Authors:
 *     Pawan Yerramilli <me@pawanyerramilli.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include <SupportDefs.h>
#include <View.h>


class SelectAreaView : public BView {
public:
							SelectAreaView(BBitmap* screenshot);

			void			AttachedToWindow();
			void			Draw(BRect updateRect);
			void			KeyDown(const char* bytes, int32 numBytes);
			void			MouseDown(BPoint point);
			void			MouseMoved(BPoint point, uint32 transit, const BMessage* message);
			void			MouseUp(BPoint point);

private:
			BRect			_CurrentFrame(bool zeroed = false);

			BBitmap*		fScreenShot;
			bool			fIsCurrentlyDragging;
			BPoint			fStartCorner;
			BPoint			fEndCorner;
};
