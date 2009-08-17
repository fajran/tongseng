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

#ifndef INCLUDED_FLASHSENDER_H
#define INCLUDED_FLASHSENDER_H

#include "OscSender.h"
//#include "ip/FlashSocket.h"

#define MAX_FLASH_SIZE 65536

namespace TUIO {
	
	/**
	 * The FlashSender implements the Flash LocalConnection transport method for OSC
	 *
	 * @author Martin Kaltenbrunner
	 * @version 1.4
	 */ 
	class LIBDECL FlashSender : public OscSender {
				
	public:

		/**
		 * The default constructor creates a FlashSender using a Flash LocalConnection on localhost
		 */
		FlashSender();

		/**
		 * The destructor closes the connection. 
		 */
		~FlashSender();
		
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
		//FlashTransmitSocket *socket;
	};
};
#endif /* INCLUDED_FLASHSENDER_H */
