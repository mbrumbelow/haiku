/*
 * Copyright 2002-2020 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef GUIDE_WINDOW_H
#define GUIDE_WINDOW_H


#include <Button.h>
#include <DiskDevice.h>
#include <DiskDeviceRoster.h>
#include <MenuField.h>
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
private:
	void				_ScanDisks();
	void				_SelectDisk();

	SizeSlider*			fSizeSlider;

	BDiskDeviceRoster		fDiskDeviceRoster;
	BMenuField*                     fDiskSelectionMenu;
	BMenuField*                     fDiskSystemMenu;
	BMenuField*                     fBootPlatformMenu;

	BButton*			fApplyButton;
	BButton*			fCancelButton;
};


#endif /* GUIDE_WINDOW_H */
