/*
 TUIO C++ Library - part of the reacTIVision project
 http://reactivision.sourceforge.net/
 
 Copyright (c) 2005-2009 Martin Kaltenbrunner <martin@tuio.org>
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef INCLUDED_TCPSENDER_H
#define INCLUDED_TCPSENDER_H

#include "OscSender.h"
//#include "ip/TcpSocket.h"

#define MAX_TCP_SIZE 65536

namespace TUIO {
	
	/**
	 * The TcpSender implements the TCP transport method for OSC
	 *
	 * @author Martin Kaltenbrunner
	 * @version 1.4
	 */ 
	class LIBDECL TcpSender : public OscSender {
				
	public:

		/**
		 * The default constructor creates a TcpSender that sends to the default TUIO port 3333 on localhost
		 */
		TcpSender();
		
		/**
		 * This constructor creates a TcpSender that sends to the provided port on the the given host
		 *
		 * @param  host  the receiving host name
		 * @param  port  the outgoing TUIO TCP port number
		 */
		TcpSender(const char *host, int port);		

		/**
		 * The destructor closes the socket. 
		 */
		~TcpSender();
		
		/**
		 * This method delivers the provided OSC data
		 *
		 * @param *bundle  the OSC stream to deliver
		 * @return true if the data was delivered successfully
		 */
		
		bool sendOSC (osc::OutboundPacketStream *bundle);

		/**
		 * This method returns the connection state
		 *
		 * @return true if the connection is alive
		 */
		bool isConnected ();
		
	private:
		//TcpTransmitSocket *socket;
	};
};
#endif /* INCLUDED_TCPSENDER_H */
