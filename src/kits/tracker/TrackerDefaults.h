/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/
#ifndef _TRACKER_DEFAULTS_H
#define _TRACKER_DEFAULTS_H


static const bool kDefaultShowDisksIcon = false;
static const bool kDefaultMountVolumesOntoDesktop = true;
static const bool kDefaultMountSharedVolumesOntoDesktop = true;
static const bool kDefaultEjectWhenUnmounting = true;

static const bool kDefaultDesktopFilePanelRoot = true;
static const bool kDefaultShowFullPathInTitleBar = false;
static const bool kDefaultShowSelectionWhenInactive = true;
static const bool kDefaultTransparentSelection = true;
static const bool kDefaultSortFolderNamesFirst = true;
static const bool kDefaultHideDotFiles = false;
static const bool kDefaultTypeAheadFiltering = false;
static const bool kDefaultGenerateImageThumbnails = true;
static const bool kDefaultSingleWindowBrowse = false;
static const bool kDefaultShowNavigator = false;

static const int32 kDefaultRecentApplications = 10;
static const int32 kDefaultRecentDocuments = 10;
static const int32 kDefaultRecentFolders = 10;

static const bool kDefaultShowVolumeSpaceBar = true;
static const uint8 kDefaultSpaceBarAlpha = 192;
static const rgb_color kDefaultUsedSpaceColor = { 0, 203, 0, kDefaultSpaceBarAlpha };
static const rgb_color kDefaultFreeSpaceColor = { 255, 255, 255, kDefaultSpaceBarAlpha };
static const rgb_color kDefaultWarningSpaceColor = { 203, 0, 0, kDefaultSpaceBarAlpha };

static const bool kDefaultDontMoveFilesToTrash = false;
static const bool kDefaultAskBeforeDeleteFile = true;


#endif	// _TRACKER_DEFAULTS_H
