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

#ifndef INCLUDED_OSCRECEIVER_H
#define INCLUDED_OSCRECEIVER_H

#include "LibExport.h"
#include "OscClient.h"

#include "osc/OscReceivedElements.h"
#include "osc/OscHostEndianness.h"
#include "ip/PacketListener.h"
#include "ip/IpEndpointName.h"
#include <list>
#include <iostream>

namespace TUIO {

	class OscClient; // Forward declaration

	/**
	 * The OscReceiver is the base class for the various OSC transport methods such as UDP, TCP ...
	 *
	 * @author Martin Kaltenbrunner
	 * @version 1.1.6
	 */ 
	class LIBDECL OscReceiver: public PacketListener {
				
	public:

		/**
		 * The constructor is doing nothing in particular. 
		 */
		OscReceiver() : connected(false) {};

		/**
		 * The destructor is doing nothing in particular. 
		 */
		virtual ~OscReceiver() {};
		
		/**
		 * The OscReceiver connects and starts receiving TUIO messages via OSC.
		 * With the default parameter the OSC receiver runs in a dedicated background thread,
		 * when set to true the method blocks in the current thread until disconnect() is invoked.
		 *
		 * @param  lock  blocks in the current thread if set to true, runs in the background if set to false (default)
		 */
		virtual void connect(bool lock=false) = 0;
		
		/**
		 * The OscReceiver disconnects and stops receiving TUIO messages via OSC.
		 * This method waits for the receiver threads to terminate and therefore
		 * must not be called from within a TuioListener callback.
		 */
		virtual void disconnect() = 0;
		
		/**
		 * Returns true if this OscReceiver is currently connected.
		 * @return	true if this OscReceiver is currently connected
		 */
		bool isConnected();
	
		/**
		 * Attaches the provided OscClient to this OscReceiver
		 *
		 * @param  client	a pointer to the OscClient to attach
		 */
		void addTuioClient(OscClient *client);
		
		/**
		 * The OSC callback method where the incoming OSC data is received
		 *
		 * @param  data		the received OSC data
		 * @param  size		the size of the received OSC data
		 * @param  remoteEndpoint	the origin of the received OSC data 
		 */
		void ProcessPacket( const char *data, int size, const IpEndpointName &remoteEndpoint );
		
	protected:
		/**
		 * Dispatches all messages of a received OSC bundle to the attached clients
		 *
		 * @param  b				the received OSC bundle
		 * @param  remoteEndpoint	the origin of the received OSC data
		 */
		void ProcessBundle( const osc::ReceivedBundle& b, const IpEndpointName& remoteEndpoint);
		
		/**
		 * Dispatches a single received OSC message to the attached clients
		 *
		 * @param  message			the received OSC message
		 * @param  remoteEndpoint	the origin of the received OSC data
		 */
		void ProcessMessage( const osc::ReceivedMessage& message, const IpEndpointName& remoteEndpoint);
		
		/** The OscClients attached to this receiver */
		std::list<OscClient*> clientList;
		
		/** Whether the receiver is currently connected */
		bool connected;
	};
};
#endif /* INCLUDED_OSCRECEIVER_H */
