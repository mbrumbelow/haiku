/*
 * Copyright 2015, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <MediaConnection.h>

#include "debug.h"


BMediaConnection::BMediaConnection(BMediaClient* owner,
	media_connection_kind kind,
	media_connection_id id)
	:
	fOwner(owner),
	fBind(NULL)
{
	CALLED();

	_Init();

	fConnection.kind = kind;
	fConnection.id = id;
	fConnection.client = fOwner->Client();

	if (IsOutput()) {
		fConnection.source.port = fOwner->fNode->ControlPort();
		fConnection.source.id = fConnection.id;

		fConnection.destination = media_destination::null;
	} else {
		// IsInput()
		fConnection.destination.port = fOwner->fNode->ControlPort();
		fConnection.destination.id = fConnection.id;

		fConnection.source = media_source::null;
	}
}


BMediaConnection::~BMediaConnection()
{
	CALLED();

}


const media_connection&
BMediaConnection::Connection() const
{
	return fConnection;
}


bool
BMediaConnection::IsOutput() const
{
	CALLED();

	return fConnection.IsOutput();
}


bool
BMediaConnection::IsInput() const
{
	CALLED();

	return fConnection.IsInput();
}


bool
BMediaConnection::HasBinding() const
{
	CALLED();

	return fBind != NULL;
}


BMediaConnection*
BMediaConnection::Binding() const
{
	CALLED();

	return fBind;
}


void
BMediaConnection::SetAcceptedFormat(const media_format& format)
{
	CALLED();

	fConnection.format = format;
}


const media_format&
BMediaConnection::AcceptedFormat() const
{
	CALLED();

	return fConnection.format;
}


bool
BMediaConnection::IsConnected() const
{
	CALLED();

	return fConnected;
}


bool
BMediaConnection::IsOutputEnabled() const
{
	CALLED();

	return fOutputEnabled;
}


void*
BMediaConnection::Cookie() const
{
	CALLED();

	return fBufferCookie;
}


status_t
BMediaConnection::Disconnect()
{
	CALLED();

	return fOwner->DisconnectConnection(this);
}


status_t
BMediaConnection::Reset()
{
	CALLED();

	delete fBufferGroup;
	fBufferGroup = NULL;

	return fOwner->ResetConnection(this);
}


status_t
BMediaConnection::Release()
{
	CALLED();

	return fOwner->ReleaseConnection(this);
}


void
BMediaConnection::SetHooks(process_hook processHook,
	notify_hook notifyHook, void* cookie)
{
	CALLED();

	fProcessHook = processHook;
	fNotifyHook = notifyHook;
	fBufferCookie = cookie;
}


void
BMediaConnection::SetBufferSize(size_t size)
{
	CALLED();

	fBufferSize = size;
}


size_t
BMediaConnection::BufferSize() const
{
	CALLED();

	return fBufferSize;
}


void
BMediaConnection::SetBufferDuration(bigtime_t duration)
{
	CALLED();

	fBufferDuration = duration;
}


bigtime_t
BMediaConnection::BufferDuration() const
{
	CALLED();

	return fBufferDuration;
}


void
BMediaConnection::ConnectedCallback(const media_source& source,
	const media_format& format)
{
	fConnection.source = source;
	SetAcceptedFormat(format);

	if (fNotifyHook != NULL)
		(*fNotifyHook)(B_CONNECTED, this);

	fConnected = true;
}


void
BMediaConnection::DisconnectedCallback(const media_source& source)
{
	if (fNotifyHook != NULL)
		(*fNotifyHook)(B_DISCONNECTED, this);

	fConnected = false;
}


void
BMediaConnection::ConnectCallback(const media_destination& destination)
{
	fConnection.destination = destination;
}


void
BMediaConnection::DisconnectCallback(const media_destination& destination)
{
}


void
BMediaConnection::_Init()
{
	CALLED();

	fBufferGroup = NULL;
	fNotifyHook = NULL;
	fProcessHook = NULL;
}


media_input
BMediaConnection::MediaInput() const
{
	return fConnection.MediaInput();
}


media_output
BMediaConnection::MediaOutput() const
{
	return fConnection.MediaOutput();
}


const media_source&
BMediaConnection::Source() const
{
	return fConnection.Source();
}


const media_destination&
BMediaConnection::Destination() const
{
	return fConnection.Destination();
}


void BMediaConnection::_ReservedMediaConnection0() {}
void BMediaConnection::_ReservedMediaConnection1() {}
void BMediaConnection::_ReservedMediaConnection2() {}
void BMediaConnection::_ReservedMediaConnection3() {}
void BMediaConnection::_ReservedMediaConnection4() {}
void BMediaConnection::_ReservedMediaConnection5() {}
void BMediaConnection::_ReservedMediaConnection6() {}
void BMediaConnection::_ReservedMediaConnection7() {}
void BMediaConnection::_ReservedMediaConnection8() {}
void BMediaConnection::_ReservedMediaConnection9() {}
void BMediaConnection::_ReservedMediaConnection10() {}