/*
 * Copyright 2022 Pascal R. G. Abresch All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef DHMO_CONTROL_LOOK_H
#define DHMO_CONTROL_LOOK_H


#include <ControlLook.h>
#include <Shape.h>

#include "HaikuControlLook.h"
namespace BPrivate {

using BPrivate::HaikuControlLook;

class DHMOControlLook : public HaikuControlLook {
public:
								DHMOControlLook();
	virtual						~DHMOControlLook();

	virtual void				DrawArrowShapeSharp(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 direction,
									uint32 flags = 0,
									float tint = B_DARKEN_MAX_TINT);

	virtual void				DrawScrollBarButton(BView* view, BRect rect,
									const BRect& updateRect, const rgb_color& base, uint32 flags,
									int32 direction, orientation orientation, bool down = false) override;

	virtual void				DrawButtonBackground(BView* view, BRect& rect,
									const BRect& updateRect, const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL) override;

	virtual void				DrawButtonFrame(BView* view, BRect& rect,
									const BRect& updateRect, const rgb_color& base,
									const rgb_color& background, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS) override;

	virtual	void				DrawButtonWithPopUpBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL) override;
									
	virtual void				DrawScrollBarBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base,
									uint32 flags,
									orientation orientation) override;

	virtual	void				DrawScrollBarThumb(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									orientation orientation,
									uint32 knobStyle = B_KNOB_DOTS) override;
};

} // namespace BPrivate

#endif	// DHMO_CONTROL_LOOK_H
