/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_PROTOCOL_ASYNCHRONOUS_LISTENER_H_
#define _B_URL_PROTOCOL_ASYNCHRONOUS_LISTENER_H_


#include <Handler.h>
#include <Message.h>
#include <UrlProtocolDispatchingListener.h>


class BURLProtocolAsynchronousListener : public BHandler,
	public BURLProtocolListener {
public:
								BURLProtocolAsynchronousListener(
									bool transparent = false);
	virtual						~BURLProtocolAsynchronousListener();

	// Synchronous listener access
			BURLProtocolListener* SynchronousListener();
									
	// BHandler interface
	virtual void				MessageReceived(BMessage* message);

private:
			BURLProtocolDispatchingListener*
						 		fSynchronousListener;
};

#endif // _B_URL_PROTOCOL_ASYNCHRONOUS_LISTENER_H_

