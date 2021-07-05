/*
 * Copyright 2008, Haiku.
 * Copyright 2021, Panagiotis Vasilopoulos <hello@alwayslivid.com>
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#ifndef _PPD_CONFIG_H
#define _PPD_CONFIG_H

#include <AppKit.h>
#include <InterfaceKit.h>
#include "PPDConfigView.h"
#include "MsgConsts.h"


extern const char* kAppName;
extern const char* kDescription;
extern const char* kSignature;


class AppWindow : public BWindow {
public:
	AppWindow(BRect);
	bool QuitRequested();
	void AboutRequested();	
	void MessageReceived(BMessage *message);
	
private:
	BMenuBar *fMenuBar;
	PPDConfigView *fConfig;
};

class PPDConfigApplication : public BApplication {
public:
	AppWindow *window;
	PPDConfigApplication();
};

#define my_app ((PPDConfigApplication*)be_app)

#endif
