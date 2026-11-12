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

#include "UdpSender.h"

using namespace TUIO;

static bool isLocalHost(const char *host) {
	return ((strcmp(host,"127.0.0.1")==0) || (strcmp(host,"localhost")==0));
}

// creates the transmit socket, or throws an exception if the host cannot be resolved
static UdpTransmitSocket* openSocket(const char *host, int port) {
	try {
		UdpTransmitSocket *socket = new UdpTransmitSocket(IpEndpointName(GetHostByName(host), port));
		std::cout << "TUIO/UDP messages to " << host << "@" << port << std::endl;
		return socket;
	} catch (std::exception &e) { 
		std::cout << "could not create UDP socket" << std::endl;
		throw std::exception();
	}
}

UdpSender::UdpSender() {
	local = true;
	buffer_size = MAX_LOCAL_UDP_SIZE;
	socket = openSocket("127.0.0.1",3333);
}

UdpSender::UdpSender(const char *host, int port) {
	local = isLocalHost(host);
	buffer_size = local ? MAX_LOCAL_UDP_SIZE : IP_MTU_SIZE;
	socket = openSocket(host,port);
}

UdpSender::UdpSender(const char *host, int port, int size) {
	local = isLocalHost(host);
	if (size>(local?MAX_LOCAL_UDP_SIZE:MAX_UDP_SIZE)) size = local?MAX_LOCAL_UDP_SIZE:MAX_UDP_SIZE;
	else if (size<MIN_UDP_SIZE) size = MIN_UDP_SIZE;
	buffer_size = size;
	socket = openSocket(host,port);
}

UdpSender::~UdpSender() {
	delete socket;		
}

bool UdpSender::isConnected() { 
	return (socket!=NULL);
}

bool UdpSender::sendOscPacket (osc::OutboundPacketStream *bundle) {
	if (socket==NULL) return false; 
	if ( bundle->Size() > buffer_size ) return false;
	if ( bundle->Size() == 0 ) return false;

	socket->Send( bundle->Data(), bundle->Size() );
	return true;
}
