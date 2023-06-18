/*
 * Copyright 2023, Trung Nguyen, trungnt282910@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef UNIX_DATAGRAM_ENDPOINT_H
#define UNIX_DATAGRAM_ENDPOINT_H


#include <Referenceable.h>

#include "UnixEndpoint.h"


class UnixFifo;


class UnixDatagramEndpoint : public UnixEndpoint, public BReferenceable {
public:
	UnixDatagramEndpoint(net_socket *socket);
	virtual ~UnixDatagramEndpoint();

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
		return fAddress.IsValid();
	}

private:
	static status_t _InitializeEndpoint(const struct sockaddr *_address,
		BReference<UnixDatagramEndpoint> &outEndpoint);

	status_t _Disconnect();
	void _UnsetReceiveFifo();

private:
	UnixDatagramEndpoint*	fTargetEndpoint;
	UnixFifo*				fReceiveFifo;
	bool					fShutdownWrite;
};


#endif	// UNIX_DATAGRAM_ENDPOINT_H
