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

#ifndef INCLUDED_OSCCLIENT_H
#define INCLUDED_OSCCLIENT_H

#include "LibExport.h"
#include "osc/OscReceivedElements.h"

namespace TUIO {

	/**
	 * The OscClient interface allows any OSC client implementation
	 * to be attached to an OscReceiver for the reception of TUIO messages.
	 *
	 * @author Martin Kaltenbrunner
	 * @version 1.1.6
	 */ 
	class LIBDECL OscClient {
				
	public:

		/**
		 * The destructor is doing nothing in particular. 
		 */
		virtual ~OscClient() {};
		
		/**
		 * Parses the provided OSC message
		 *
		 * @param  message  the received OSC message
		 */
		virtual void processOSC( const osc::ReceivedMessage& message ) = 0;

		/**
		 * Called when the receiver starts processing a new OSC packet,
		 * allowing the client to discard any pending frame state
		 * left over from a previous incomplete or malformed bundle
		 */
		virtual void processBundleBegin() = 0;
	};
};
#endif /* INCLUDED_OSCCLIENT_H */
