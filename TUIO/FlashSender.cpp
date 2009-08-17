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


#include "FlashSender.h"

using namespace TUIO;

FlashSender::FlashSender() {
	try {
		local = true;
		//socket = new FlashTransmitSocket();
		buffer_size = MAX_FLASH_SIZE;
	} catch (std::exception &e) { 
		std::cout << "could not create connection" << std::endl;
		//socket = NULL;
	}
}

FlashSender::~FlashSender() {
	//delete socket;		
}

bool FlashSender::isConnected() { 
	//if (socket==NULL) return false; 
	return true;
}

bool FlashSender::sendOSC (osc::OutboundPacketStream *bundle) {
	//if (socket==NULL) return false; 
	if ( bundle->Size() > buffer_size ) return false;
	//socket->Send( bundle->Data(), bundle->Size() );
	return true;
}
