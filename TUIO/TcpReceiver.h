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

#ifndef INCLUDED_TCPRECEIVER_H
#define INCLUDED_TCPRECEIVER_H

#include "OscReceiver.h"
#define MAX_TCP_SIZE 65536

#ifdef WIN32
#include <winsock.h>
#include <io.h>
#include <stdint.h>
typedef int socklen_t;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

namespace TUIO {
	
	/**
	 * The TcpReceiver provides the OscReceiver functionality for the TCP transport method 
	 *
	 * @author Martin Kaltenbrunner
	 * @version 1.1.6
	 */ 
	class LIBDECL TcpReceiver: public OscReceiver {
				
	public:
		
		/**
		 * This constructor creates a TcpReceiver instance listening to the provided TCP port 
		 *
		 * @param  port  the number of the TCP port to listen to, defaults to 3333
		 */
		TcpReceiver (int port=3333);

		/**
		 * This constructor creates a TcpReceiver connected to the provided host and TCP port 
		 *
		 * @param  host  the host name to connect to
		 * @param  port  the number of the TCP port to connect to, defaults to 3333
		 */
		TcpReceiver (const char *host, int port);
		
		/**
		 * The destructor stops the receiver and closes the socket. 
		 */
		virtual ~TcpReceiver();
		
		/**
		 * The TcpReceiver connects and starts receiving TUIO messages via TCP.
		 * With the default parameter the OSC receiver runs in a dedicated background thread,
		 * when set to true the method blocks in the current thread until disconnect() is invoked.
		 *
		 * @param  lock  blocks in the current thread if set to true, runs in the background if set to false (default)
		 */
		void connect(bool lock=false);
		
		/**
		 * The TcpReceiver disconnects and stops receiving TUIO messages via TCP
		 */
		void disconnect();

#ifndef WIN32
		/** The listening socket, public to be accessible from the thread function */
		int tcp_socket;
		/** The list of connected client sockets */
		std::list<int> tcp_client_list;
		/** The mutex protecting tcp_client_list */
		pthread_mutex_t tcp_mutex;
#else
		/** The listening socket, public to be accessible from the thread function */
		SOCKET tcp_socket;
		/** The list of connected client sockets */
		std::list<SOCKET> tcp_client_list;
		/** The mutex protecting tcp_client_list */
		HANDLE tcp_mutex;
#endif
		
	private:

#ifndef WIN32
		pthread_t server_thread;
#else
		HANDLE server_thread;
		DWORD ServerThreadId;
#endif	
		
		bool locked;
		bool local;
	};
};
#endif /* INCLUDED_TcpReceiver_H */
