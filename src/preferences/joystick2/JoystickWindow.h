/*
 * Copyright 2020, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#ifndef JOYSTICK_WINDOW_H
#define JOYSTICK_WINDOW_H


#include <Box.h>
#include <CardView.h>
#include <Input.h>
#include <ListItem.h>
#include <ListView.h>
#include <Message.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <View.h>
#include <Window.h>

#include "JoystickView.h"


class JoystickWindow : public BWindow
{
public:
						JoystickWindow(BRect rect);
		void				Show();
		void				Hide();

private:
		JoystickView*	fJoystickView;
};

#endif /* JOYSTICK_WINDOW_H */
