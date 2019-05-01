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
#include <StringView.h>
#include <View.h>
#include <GroupView.h>


class InputWindow : public BWindow {
public:
		InputWindow(BRect rect);
};

#endif	/* INPUT_WINDOW_H */
