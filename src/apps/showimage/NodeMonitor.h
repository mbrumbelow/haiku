/*
 * Copyright 2022, Javier Steinaker <jsteinaker@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _SHOWIMAGE_NODEMONITOR_H
#define _SHOWIMAGE_NODEMONITOR_H

#include <map>

#include <Handler.h>
#include <Entry.h>
#include <Node.h>

class NodeMonitor : public BHandler
{
	public:
						NodeMonitor();
						~NodeMonitor();

		void 			AddWatch(BMessage *message);
		void 			RemoveWatch(BMessage *message);
		int32 			WatchCount() { return fWatchCount; }

	protected:
		virtual void	MessageReceived(BMessage *message = NULL);

	private:
		typedef std::map<node_ref, entry_ref> NodeMap;

		int32			fWatchCount;
		NodeMap			fNodeMap;
};


#endif // _SHOWIMAGE_NODEMONITOR_H
