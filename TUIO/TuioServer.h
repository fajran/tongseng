/*
 TUIO Server Component - part of the reacTIVision project
 http://reactivision.sourceforge.net/
 
 Copyright (C) 2005-2009 Martin Kaltenbrunner <martin@tuio.org>
 
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

#ifndef INCLUDED_TuioServer_H
#define INCLUDED_TuioServer_H

#include "TuioManager.h"
#include "UdpSender.h"
#include <stdio.h>
#ifndef WIN32
#include <netdb.h>
#include <arpa/inet.h>
#endif

namespace TUIO {
	/**
	 * <p>The TuioServer class is the central TUIO protocol encoder component.
	 * In order to encode and send TUIO messages an instance of TuioServer needs to be created. The TuioServer instance then generates TUIO messaged
	 * which are sent via OSC over UDP to the configured IP address and port.</p> 
	 * <p>During runtime the each frame is marked with the initFrame and commitFrame methods, 
	 * while the currently present TuioObjects are managed by the server with ADD, UPDATE and REMOVE methods in analogy to the TuioClient's TuioListener interface.</p> 
	 * <p><code>
	 * TuioClient *server = new TuioServer();<br/>
	 * ...<br/>
	 * server->initFrame(TuioTime::getSessionTime());<br/>
	 * TuioObject *tobj = server->addTuioObject(xpos,ypos, angle);<br/>
	 * TuioCursor *tcur = server->addTuioObject(xpos,ypos);<br/>
	 * server->commitFrame();<br/>
	 * ...<br/>
	 * server->initFrame(TuioTime::getSessionTime());<br/>
	 * server->updateTuioObject(tobj, xpos,ypos, angle);<br/>
	 * server->updateTuioCursor(tcur, xpos,ypos);<br/>
	 * server->commitFrame();<br/>
	 * ...<br/>
	 * server->initFrame(TuioTime::getSessionTime());<br/>
	 * server->removeTuioObject(tobj);<br/>
	 * server->removeTuioCursor(tcur);<br/>
	 * server->commitFrame();<br/>
	 * </code></p>
	 *
	 * @author Martin Kaltenbrunner
	 * @version 1.4
	 */ 
	class LIBDECL TuioServer : public TuioManager { 
	
	public:

		/**
		 * This constructor creates a TuioServer that sends to the provided port on the the given host
		 * using a default packet size of 1492 bytes to deliver unfragmented UDP packets on a LAN
		 *
		 * @param  host  the receiving host name
		 * @param  port  the outgoing TUIO UDP port number
		 */
		TuioServer(OscSender *sender);

		/**
		 * The destructor is doing nothing in particular. 
		 */
		~TuioServer();

		/**
		 * Enables the full update of all currently active and inactive TuioObjects, TuioCursors and TuioBlobs 
		 *
		 */
		void enableFullUpdate()  {
			full_update = true;
		}
		
		/**
		 * Disables the full update of all currently active and inactive TuioObjects, TuioCursors and TuioBlobs 
		 */
		void disableFullUpdate() {
			full_update = false;
		}

		/**
		 * Returns true if the full update of all currently active TuioObjects, TuioCursors and TuioBlobs is enabled.
		 * @return	true if the full update of all currently active TuioObjects, TuioCursors and TuioBlobs is enabled
		 */
		bool fullUpdateEnabled() {
			return full_update;
		}

		/**
		 * Disables the periodic full update of all currently active TuioObjects TuioObjects, TuioCursors and TuioBlobs
		 *
		 * @param	interval	update interval in seconds, defaults to one second
		 */
		void enablePeriodicMessages(int interval=1) {
			periodic_update =  true;
			update_interval = interval;
		}
		
		/**
		 * Disables the periodic full update of all currently active and inactive TuioObjects, TuioCursors and TuioBlobs
		 */
		void disablePeriodicMessages() {
			periodic_update = false;
		}
		
		/**
		 * Returns true if the periodic update of all currently active TuioObjects, TuioCursors and TuioBlobs is enabled.
		 * @return	true if the periodic update of all currently active TuioObjects, TuioCursors and TuioBlobs is enabled
		 */
		bool periodicMessagesEnabled() {
			return periodic_update;
		}
		
		/**
		 * Returns the periodic update interval in seconds.
		 * @return	the periodic update interval in seconds
		 */
		int getUpdateInterval() {
			return update_interval;
		}
		
		/**
		 * Commits the current frame.
		 * Generates and sends TUIO messages of all currently active and updated TuioObjects, TuioCursors and TuioBlobs.
		 */
		void commitFrame();

		/**
		 * Commits the current frame.
		 * Generates and sends TUIO messages of all currently active and updated TuioObjects, TuioCursors and TuioBlobs.
		 */
		
		/**
		 * Defines the name of this TUIO source, which is transmitted within the /tuio/[profile] source message.
		 *
		 * @param	src	the desired name of this TUIO source
		 */
		void setSourceName(const char *src);
		
		void enableObjectProfile(bool flag) { objectProfileEnabled = flag; };
		void enableCursorProfile(bool flag) { cursorProfileEnabled = flag; };
		void enableBlobProfile(bool flag) { blobProfileEnabled = flag; };
				
	private:
				
		OscSender *sender;	
		osc::OutboundPacketStream  *oscPacket;
		char *oscBuffer; 
				
		void startObjectBundle();
		void addObjectMessage(TuioObject *tobj);
		void sendObjectBundle(long fseq);
		void sendEmptyObjectBundle();

		void startCursorBundle();
		void addCursorMessage(TuioCursor *tcur);
		void sendCursorBundle(long fseq);
		void sendEmptyCursorBundle();

		void startBlobBundle();
		void addBlobMessage(TuioBlob *tblb);
		void sendBlobBundle(long fseq);
		void sendEmptyBlobBundle();
		
		int update_interval;
		bool full_update, periodic_update;
		TuioTime objectUpdateTime, cursorUpdateTime, blobUpdateTime ;
		bool objectProfileEnabled, cursorProfileEnabled, blobProfileEnabled;		
		char *source_name;
	};
};
#endif /* INCLUDED_TuioServer_H */
