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

#ifndef INCLUDED_TUIOCLIENT_H
#define INCLUDED_TUIOCLIENT_H

#include "TuioDispatcher.h"
#include "OscReceiver.h"
#include "osc/OscReceivedElements.h"

#include <iostream>
#include <list>
#include <map>
#include <algorithm>
#include <string>
#include <cstring>

namespace TUIO {
	
	class OscReceiver; // Forward declaration
	
	/**
	 * <p>The TuioClient class is the central TUIO protocol decoder component. It provides a simple callback infrastructure using the TuioListener interface.
	 * In order to receive and decode TUIO messages an instance of TuioClient needs to be created. The TuioClient instance then generates TUIO events
	 * which are broadcasted to all registered classes that implement the TuioListener interface.</p>
	 * <p>The TuioObject, TuioCursor and TuioBlob references returned by the list getter methods are only valid
	 * until the referenced component is removed from the session. Use the copyTuioObjects(), copyTuioCursors()
	 * and copyTuioBlobs() methods to retrieve an owned snapshot copy of the currently active components.</p> 
	 * <p><code>
	 * TuioClient *client = new TuioClient();<br/>
	 * client->addTuioListener(myTuioListener);<br/>
	 * client->connect();<br/>
	 * </code></p>
	 *
	 * @author Martin Kaltenbrunner
	 * @version 1.1.6
	 */ 
	class LIBDECL TuioClient : public TuioDispatcher, public OscClient { 
		
	public:
		/**
		 * This constructor creates a TuioClient that uses an internal UdpReceiver listening to the default UDP port 3333
		 *
		 */
		TuioClient();
	
		/**
		 * This constructor creates a TuioClient that uses an internal UdpReceiver listening to the provided UDP port
		 *
		 * @param  port  the UDP port the internal UdpReceiver is listening to
		 */
		TuioClient(int port);
		
		/**
		 * This constructor creates a TuioClient that uses the provided OscReceiver for the incoming OSC data.
		 * The OscReceiver instance remains owned by the caller and is not deleted by this TuioClient.
		 *
		 * @param  oscreceiver  the OscReceiver implementation for the chosen transport method (UDP, TCP ...)
		 */
		TuioClient(OscReceiver *oscreceiver);

		/**
		 * The destructor is doing nothing in particular. 
		 */
		~TuioClient();

		/**
		 * The TuioClient connects and starts receiving TUIO messages from its associated OscReceiver.
		 * With the default parameter the OSC receiver runs in a dedicated background thread,
		 * when set to true the method blocks in the current thread until disconnect() is invoked.
		 *
		 * @param  lock  blocks in the current thread if set to true, runs in the background if set to false (default)
		 */
		void connect(bool lock=false);
		
		/**
		 * The TuioClient disconnects and stops receiving TUIO messages from its associated OscReceiver
		 */
		void disconnect();
		
		/**
		 * Returns true if this TuioClient is currently connected.
		 * @return	true if this TuioClient is currently connected
		 */
		bool isConnected();

		/**
		 * Returns a List of all currently active TuioObjects
		 *
		 * @return  a List of TuioObjects
		 */
		std::list<TuioObject*> getTuioObjects() {
			return TuioDispatcher::getTuioObjects();
		}
		
		/**
		 * Returns a List of all currently active TuioObjects
		 * which are associated to the given Source ID
		 *
		 * @param  source_id  the source ID of the corresponding TUIO source
		 * @return  a List of TuioObjects
		 */
		std::list<TuioObject*> getTuioObjects(int source_id);

		/**
		 * Returns a List with a copy of all currently active TuioObjects
		 * which are associated to the given Source ID
		 *
		 * @param  source_id  the source ID of the corresponding TUIO source
		 * @return  a List with a copy of TuioObjects
		 */
		std::list<TuioObject> copyTuioObjects(int source_id);

		
		/**
		 * Returns a List with a copy of all currently active TuioObjects
		 *
		 * @return  a List with a copy of TuioObjects
		 */
		std::list<TuioObject> copyTuioObjects() {
			return TuioDispatcher::copyTuioObjects();
		}
		
		/**
		 * Returns the TuioObject corresponding to the provided Session ID
		 * or NULL if the Session ID does not refer to an active TuioObject
		 *
		 * @param  s_id  the session ID of the corresponding TuioObject
		 * @return  an active TuioObject corresponding to the provided Session ID or NULL
		 */
		TuioObject* getTuioObject(int s_id) {
			return getTuioObject(0,s_id);
		};
		
		/**
		 * Returns the TuioObject corresponding to the provided Session ID
		 * which is associated to the given Source ID
		 * or NULL if the Session ID does not refer to an active TuioObject
		 *
		 * @param  src_id  the source ID of the corresponding TUIO source
		 * @param  s_id  the session ID of the corresponding TuioObject
		 * @return  an active TuioObject corresponding to the provided Session ID or NULL
		 */
		TuioObject* getTuioObject(int src_id, int s_id);

		/**
		 * Returns a List of all currently active TuioCursors
		 *
		 * @return  a List of TuioCursors
		 */
		std::list<TuioCursor*> getTuioCursors() {
			return TuioDispatcher::getTuioCursors();
		}
		
		/**
		 * Returns a List of all currently active TuioCursors
		 * which are associated to the given Source ID
		 *
		 * @param  source_id  the source ID of the corresponding TUIO source
		 * @return  a List of TuioCursors
		 */
		std::list<TuioCursor*> getTuioCursors(int source_id);

		/**
		 * Returns a List with a copy of all currently active TuioCursors
		 *
		 * @return  a List with a copy of TuioCursors
		 */
		std::list<TuioCursor> copyTuioCursors() {
			return TuioDispatcher::copyTuioCursors();
		}
		
		/**
		 * Returns a List with a copy of all currently active TuioCursors
		 * which are associated to the given Source ID
		 *
		 * @param  source_id  the source ID of the corresponding TUIO source
		 * @return  a List with a copy of TuioCursors
		 */
		std::list<TuioCursor> copyTuioCursors(int source_id);
		
		/**
		 * Returns the TuioCursor corresponding to the provided Session ID
		 * or NULL if the Session ID does not refer to an active TuioCursor
		 *
		 * @param  s_id  the session ID of the corresponding TuioCursor
		 * @return  an active TuioCursor corresponding to the provided Session ID or NULL
		 */
		TuioCursor* getTuioCursor(int s_id) {
			return getTuioCursor(0,s_id);
		};
		
		/**
		 * Returns the TuioCursor corresponding to the provided Session ID
		 * which is associated to the given Source ID
		 * or NULL if the Session ID does not refer to an active TuioCursor
		 *
		 * @param  src_id  the source ID of the corresponding TUIO source
		 * @param  s_id  the session ID of the corresponding TuioCursor
		 * @return  an active TuioCursor corresponding to the provided Session ID or NULL
		 */
		TuioCursor* getTuioCursor(int src_id, int s_id);

		/**
		 * Returns a List of all currently active TuioBlobs
		 *
		 * @return  a List of TuioBlobs
		 */
		std::list<TuioBlob*> getTuioBlobs() {
			return TuioDispatcher::getTuioBlobs();
		}
		
		/**
		 * Returns a List of all currently active TuioBlobs
		 * which are associated to the given Source ID
		 *
		 * @param  source_id  the source ID of the corresponding TUIO source
		 * @return  a List of TuioBlobs
		 */
		std::list<TuioBlob*> getTuioBlobs(int source_id);

		/**
		 * Returns a List with a copy of all currently active TuioBlobs
		 *
		 * @return  a List with a copy of TuioBlobs
		 */
		std::list<TuioBlob> copyTuioBlobs() {
			return TuioDispatcher::copyTuioBlobs();
		}
		
		/**
		 * Returns a List with a copy of all currently active TuioBlobs
		 * which are associated to the given Source ID
		 *
		 * @param  source_id  the source ID of the corresponding TUIO source
		 * @return  a List with a copy of TuioBlobs
		 */
		std::list<TuioBlob> copyTuioBlobs(int source_id);
		
		/**
		 * Returns the TuioBlob corresponding to the provided Session ID
		 * or NULL if the Session ID does not refer to an active TuioBlob
		 *
		 * @param  s_id  the session ID of the corresponding TuioBlob
		 * @return  an active TuioBlob corresponding to the provided Session ID or NULL
		 */
		TuioBlob* getTuioBlob(int s_id) {
			return getTuioBlob(0,s_id);
		};
		
		/**
		 * Returns the TuioBlob corresponding to the provided Session ID
		 * which is associated to the given Source ID
		 * or NULL if the Session ID does not refer to an active TuioBlob
		 *
		 * @param  src_id  the source ID of the corresponding TUIO source
		 * @param  s_id  the session ID of the corresponding TuioBlob
		 * @return  an active TuioBlob corresponding to the provided Session ID or NULL
		 */
		TuioBlob* getTuioBlob(int src_id, int s_id);
		
		/**
		 * Processes a single received OSC message. This method is invoked by the associated
		 * OscReceiver for every incoming TUIO message and is usually not called directly.
		 *
		 * @param  message  the OSC message to process
		 */
		void processOSC( const osc::ReceivedMessage& message);

		/**
		 * Resets the pending frame state before a new OSC packet is processed.
		 * This method is invoked by the associated OscReceiver once per packet
		 * and discards pending set data from an incomplete previous bundle.
		 * It is usually not called directly.
		 */
		void processBundleBegin();

	private:
		void initialize();
		
		std::list<TuioObject*> frameObjects;
		std::list<int> aliveObjectList;
		std::list<TuioCursor*> frameCursors;
		std::list<int> aliveCursorList;
		std::list<TuioBlob*> frameBlobs;
		std::list<int> aliveBlobList;
		
		int currentFrame;
		TuioTime currentTime;
			
		std::list<TuioCursor*> freeCursorList, freeCursorBuffer;
		std::map<int,int> maxCursorID;

		std::list<TuioBlob*> freeBlobList, freeBlobBuffer;
		std::map<int,int> maxBlobID;
		
		std::map<std::string,int> sourceList;
		int source_id;
		std::string source_name;
		std::string source_addr;
		
		OscReceiver *receiver;
		bool local_receiver;
	};
};
#endif /* INCLUDED_TUIOCLIENT_H */
