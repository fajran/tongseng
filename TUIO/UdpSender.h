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

#ifndef INCLUDED_UDPSENDER_H
#define INCLUDED_UDPSENDER_H

#include "OscSender.h"
#include "ip/UdpSocket.h"

#define IP_MTU_SIZE 1500
#define MAX_UDP_SIZE 4096
#define MIN_UDP_SIZE 576
#define MAX_LOCAL_UDP_SIZE 65507

namespace TUIO {
	
	/**
	 * The UdpSender implements the UDP transport method for OSC
	 *
	 * @author Martin Kaltenbrunner
	 * @version 1.1.6
	 */ 
	class LIBDECL UdpSender : public OscSender {
				
	public:

		/**
		 * The default constructor creates a UdpSender that sends to the default UDP port 3333 on localhost
		 * using the maximum packet size of 65507 bytes (MAX_LOCAL_UDP_SIZE) on the loopback device
		 */
		UdpSender();
		
		/**
		 * This constructor creates a UdpSender that sends to the provided port on the given host
		 * using the default MTU size of 1500 bytes to deliver unfragmented UDP packets on a LAN,
		 * or the maximum packet size of 65507 bytes if the host is localhost or 127.0.0.1
		 *
		 * @param  host  the receiving host name
		 * @param  port  the outgoing UDP port number
		 */
		UdpSender(const char *host, int port);
		
		/**
		 * This constructor creates a UdpSender that sends to the provided port on the given host,
		 * the UDP packet size is clamped to a value between 576 (MIN_UDP_SIZE) and 4096 (MAX_UDP_SIZE) bytes,
		 * or to a value between 576 (MIN_UDP_SIZE) and 65507 (MAX_LOCAL_UDP_SIZE) bytes on the loopback device.
		 * Note that packets larger than the MTU size of 1500 bytes may be fragmented on remote networks.
		 *
		 * @param  host  the receiving host name
		 * @param  port  the outgoing UDP port number
		 * @param  size  the maximum UDP packet size
		 */
		UdpSender(const char *host, int port, int size);

		/**
		 * The destructor closes the socket. 
		 */
		virtual ~UdpSender();
		
		/**
		 * This method delivers the provided OSC data
		 *
		 * @param *bundle  the OSC stream to deliver
		 * @return true if the data was delivered successfully
		 */
		
		bool sendOscPacket (osc::OutboundPacketStream *bundle);

		/**
		 * This method returns the connection state
		 *
		 * @return true if the connection is alive
		 */
		 bool isConnected ();
		
		 const char* tuio_type() { return "TUIO/UDP"; }
		
	private:
		UdpTransmitSocket *socket;
	};
}
#endif /* INCLUDED_UDPSENDER_H */
