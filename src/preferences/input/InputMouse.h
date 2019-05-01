/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
*/


#ifndef INPUT_MOUSE_H
#define INPUT_MOUSE_H


#include <Box.h>
#include <Bitmap.h>
#include <Button.h>
#include <PopUpMenu.h>


class MousePref : public BBox {
public:
		            MousePref(const char* name, uint32 flags);
	virtual         ~MousePref();
private:
	friend class InputWindow;

	typedef BBox inherited;
    
    BPopUpMenu*     fTypeMenu;
};

#endif	/* INPUT_MOUSE_H */
