/*
 * Copyright 2014, Adrien Destugues <pulkomandy@pulkomandy.tk>.
 * Distributed under the terms of the MIT License.
 */
#ifndef BOOKMARK_BAR_H
#define BOOKMARK_BAR_H


#include <map>

#include <InterfaceKit.h> 
#include <MenuBar.h>
#include <Node.h>
#include <NodeMonitor.h>
#include <Size.h>
#include<PopUpMenu.h>


class BEntry;

namespace BPrivate {
	class IconMenuItem;
}



class BookmarkBar: public BMenuBar {
public:
									BookmarkBar(const char* title,
										BHandler* target,
										const entry_ref* navDir);
									~BookmarkBar();
	void							_ContextMenu(BPoint where);
	void							AttachedToWindow();
	void							MessageReceived(BMessage* message);
	void 							MouseDown(BPoint where);
	int32							IndexOfmenu(BPoint point);
	void							FrameResized(float width, float height);
	BSize							MinSize();

private:
	void							_AddItem(ino_t inode, BEntry* entry);
private:
	node_ref						fNodeRef;
	std::map<ino_t, BPrivate::IconMenuItem*>	fItemsMap;
	BMenu*							fOverflowMenu;
	
	// True if fOverflowMenu is currently added to BookmarkBar
	bool							fOverflowMenuAdded;
	BPopUpMenu*						popupMenu;
};


#endif // BOOKMARK_BAR_H
