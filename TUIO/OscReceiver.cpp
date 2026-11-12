/*
 TUIO C++ Library
 Copyright (c) 2005-2026 Martin Kaltenbrunner <martin@tuio.org>
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 3.0 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library.
*/

#include "OscReceiver.h"

using namespace TUIO;
using namespace osc;

void OscReceiver::ProcessMessage( const ReceivedMessage& msg, const IpEndpointName& remoteEndpoint) {
	for (std::list<OscClient*>::iterator client=clientList.begin(); client!= clientList.end(); client++)
		(*client)->processOSC(msg);
}
void OscReceiver::ProcessBundle( const ReceivedBundle& b, const IpEndpointName& remoteEndpoint) {

	// the element size table was already validated by the ReceivedBundle
	// constructor — catch per element so one malformed message or nested
	// bundle does not abort the remaining elements of this bundle
	for( ReceivedBundle::const_iterator i = b.ElementsBegin(); i != b.ElementsEnd(); ++i ){
		try {
			if( i->IsBundle() )
				ProcessBundle( ReceivedBundle(*i), remoteEndpoint);
			else
				ProcessMessage( ReceivedMessage(*i), remoteEndpoint);
		} catch (Exception& e) {
			std::cerr << "malformed OSC data: " << e.what() << std::endl;
		}
	}
}

void OscReceiver::ProcessPacket( const char *data, int size, const IpEndpointName& remoteEndpoint ) {
	if (!data) return;
	try {
		for (std::list<OscClient*>::iterator client=clientList.begin(); client!= clientList.end(); client++)
			(*client)->processBundleBegin();

		ReceivedPacket p( data, size );
		if(p.IsBundle()) ProcessBundle( ReceivedBundle(p), remoteEndpoint);
		else ProcessMessage( ReceivedMessage(p), remoteEndpoint);
	} catch (Exception& e) {
		std::cerr << "malformed OSC data: " << e.what() << std::endl;
	}
}

bool OscReceiver::isConnected() {
	return connected;
}

void OscReceiver::addTuioClient(OscClient *client) {
	clientList.push_back(client);
}

