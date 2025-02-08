/*
 * Copyright (c) 2007-2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef _TRACKER_ADDON_H
#define _TRACKER_ADDON_H

struct entry_ref;
class BMessage;
class BMenu;
class BHandler;

extern "C" {
	static const uint32 be_message_load_addon = 'Tlda';
	static const uint32 be_message_addon_loaded = 'Taol';
	void process_refs(entry_ref directory, BMessage* refs, void* reserved);
	void populate_menu(BMessage* msg, BMenu* menu, BHandler* handler);
	void message_received(BMessage* msg);
}

static const uint32 B_MESSAGE_LOAD_ADDON = be_message_load_addon;
static const uint32 B_MESSAGE_ADDON_LOADED = be_message_addon_loaded;

#endif	// _TRACKER_ADDON_H
