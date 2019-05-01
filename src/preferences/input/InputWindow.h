/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#ifndef INPUT_WINDOW_H
#define INPUT_WINDOW_H

#include <Window.h>


class InputWindow : public BWindow {
public:
		InputWindow(BRect rect);

		virtual bool QuitRequested();
};

#endif	/* INPUT_WINDOW_H */
