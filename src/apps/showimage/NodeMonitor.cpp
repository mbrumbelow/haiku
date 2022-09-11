/*
 * Copyright 2022, Javier Steinaker <jsteinaker@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <NodeMonitor.h>

#include "NodeMonitor.h"
#include "ShowImageApp.h"


NodeMonitor::NodeMonitor() : BHandler("NodeMonitor"),
	fWatchCount(0)
{
	my_app->AddHandler(this);
}


NodeMonitor::~NodeMonitor()
{
}


void NodeMonitor::AddWatch(BMessage *message)
{
	if (message == NULL)
		return;

	node_ref nodeRef;
	entry_ref entryRef;

	if (message->FindRef("entryref", &entryRef) != B_OK ||
		message->FindNodeRef("noderef", &nodeRef) != B_OK)
		return;


	fNodeMap.insert({nodeRef, entryRef});

	if (watch_node(&nodeRef, B_WATCH_ALL, this) == B_OK) {
		fWatchCount += 1;
	}
}


void NodeMonitor::RemoveWatch(BMessage *message)
{
	if (message == NULL)
		return;

	node_ref nodeRef;
	entry_ref entryRef;
	if (message->FindRef("entryref", &entryRef) != B_OK ||
	message->FindNodeRef("noderef", &nodeRef) != B_OK)
		return;

	if(watch_node(&nodeRef, B_STOP_WATCHING, this) == B_OK) {
		fWatchCount -= 1;
	}
	
	fNodeMap.erase(nodeRef);
}


void NodeMonitor::MessageReceived(BMessage *message)
{
	if (message == NULL || message->what != B_NODE_MONITOR)
		return;

	int32 opcode;
	message->FindInt32("opcode", &opcode);

	switch(opcode)
	{
		// Timestamps have changed, size has changed, or the file was
		// removed: in any case, we want to delete the cache.
		case B_STAT_CHANGED:
		case B_ENTRY_REMOVED:
		{
			node_ref nodeRef;
			message->FindInt32("device", &nodeRef.device);
			message->FindInt64("node", &nodeRef.node);
			NodeMap::iterator find = fNodeMap.find(nodeRef);
			if (find != fNodeMap.end())
			{
				entry_ref entryRef = find->second;
				my_app->DefaultCache().RemoveEntry(entryRef, nodeRef, 1);
			}
			break;
		}
		
		// If the file was moved, we keep the cache and just update the reference
		// for it.
		case B_ENTRY_MOVED:
		{
			node_ref nodeRef;
			message->FindInt32("device", &nodeRef.device);
			message->FindInt64("node", &nodeRef.node);
			NodeMap::iterator find = fNodeMap.find(nodeRef);
			if (find != fNodeMap.end())
			{
				entry_ref entryRef;
				const char* name;
				message->FindInt32("device", &entryRef.device);
				message->FindInt64("to directory", &entryRef.directory);
				message->FindString("name", &name);
				entryRef.set_name(name);
				find->second = entryRef;
			}
			
			break;
		}
		default:
			break;
	}
}
