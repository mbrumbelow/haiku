/*
 * Copyright 2002-2020 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef GUIDE_WINDOW_H
#define GUIDE_WINDOW_H


#include <DiskDevice.h>
#include <DiskDeviceRoster.h>
#include <Window.h>

#include "Support.h"


class GuideWindow : public BWindow {
public:
								GuideWindow();
	virtual						~GuideWindow();

	// BWindow interface
	virtual bool			    QuitRequested();
	virtual void			    MessageReceived(BMessage* message);

			BDiskDevice*		fCurrentDisk;
};


#endif /* GUIDE_WINDOW_H */
