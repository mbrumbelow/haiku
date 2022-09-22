/*
 * Copyright 2017-2019, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef SERVER_HELPER_H
#define SERVER_HELPER_H

#include <HttpSession.h>
#include "ValidationFailure.h"

class BMessage;
namespace BPrivate
{
	namespace Network {
		class BHttpFields;
	}
}
using BPrivate::Network::BHttpSession;


class ServerHelper {
public:
	static	bool						IsNetworkAvailable();
	static	bool						IsPlatformNetworkAvailable();

	static	void						NotifyClientTooOld(
											const BPrivate::Network::BHttpFields& responseFields
											);
	static	void						AlertClientTooOld(BMessage* message);

	static	void						NotifyTransportError(status_t error);
	static	void						AlertTransportError(BMessage* message);

	static	void						NotifyServerJsonRpcError(
											BMessage& error);
	static	void						AlertServerJsonRpcError(
											BMessage* responseEnvelopeMessage);
	static	void						GetFailuresFromJsonRpcError(
											ValidationFailures& failures,
											BMessage& responseEnvelopeMessage);
	static	BHttpSession				GetHttpSession();

private:
	static	BHttpSession				sSession;
};

#endif // SERVER_HELPER_H
