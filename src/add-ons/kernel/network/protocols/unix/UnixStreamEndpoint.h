/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UNIX_STREAM_ENDPOINT_H
#define UNIX_STREAM_ENDPOINT_H

#include <sys/stat.h>

#include <Referenceable.h>

#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>

#include "unix.h"
#include "UnixEndpoint.h"

class UnixStreamEndpoint;
class UnixFifo;


enum unix_stream_endpoint_state {
	UNIX_STREAM_ENDPOINT_NOT_CONNECTED,
	UNIX_STREAM_ENDPOINT_LISTENING,
	UNIX_STREAM_ENDPOINT_CONNECTED,
	UNIX_STREAM_ENDPOINT_CLOSED
};


typedef AutoLocker<UnixStreamEndpoint> UnixStreamEndpointLocker;


class UnixStreamEndpoint : public UnixEndpoint, public BReferenceable {
public:
	UnixStreamEndpoint(net_socket *socket);
	virtual ~UnixStreamEndpoint();

	status_t Init();
	void Uninit();

	status_t Open();
	status_t Close();
	status_t Free();

	status_t Bind(const struct sockaddr *_address);
	status_t Unbind();
	status_t Listen(int backlog);
	status_t Connect(const struct sockaddr *address);
	status_t Accept(net_socket **_acceptedSocket);

	ssize_t Send(const iovec *vecs, size_t vecCount,
		ancillary_data_container *ancillaryData,
		const struct sockaddr *address, socklen_t addressLength);
	ssize_t Receive(const iovec *vecs, size_t vecCount,
		ancillary_data_container **_ancillaryData, struct sockaddr *_address,
		socklen_t *_addressLength);

	ssize_t Sendable();
	ssize_t Receivable();

	status_t SetReceiveBufferSize(size_t size);
	status_t GetPeerCredentials(ucred *credentials);

	status_t Shutdown(int direction);

	bool IsBound() const
	{
		return !fIsChild && fAddress.IsValid();
	}

private:
	void _Spawn(UnixStreamEndpoint* connectingEndpoint,
		UnixStreamEndpoint* listeningEndpoint, UnixFifo* fifo);
	void _Disconnect();
	status_t _LockConnectedEndpoints(UnixStreamEndpointLocker& locker,
		UnixStreamEndpointLocker& peerLocker);

	status_t _Unbind();

	void _UnsetReceiveFifo();
	void _StopListening();

private:
	UnixStreamEndpoint*				fPeerEndpoint;
	UnixFifo*						fReceiveFifo;
	unix_stream_endpoint_state		fState;
	sem_id							fAcceptSemaphore;
	ucred							fCredentials;
	bool							fIsChild;
	bool							fWasConnected;
};

#endif	// UNIX_STREAM_ENDPOINT_H
