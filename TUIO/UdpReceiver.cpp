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

#include "UdpReceiver.h"

#ifndef WIN32
#include <unistd.h>
#endif

using namespace TUIO;
using namespace osc;

#ifndef WIN32
static void* ClientThreadFunc( void* obj )
#else
static DWORD WINAPI ClientThreadFunc( LPVOID obj )
#endif
{
	UdpReceiver *receiver = static_cast<UdpReceiver*>(obj);
	receiver->socket->Run();
	receiver->thread_done = true;
	return 0;
};

UdpReceiver::UdpReceiver(int port):thread_done (true),locked (false) {
	try {
		socket = new UdpListeningReceiveSocket(IpEndpointName( IpEndpointName::ANY_ADDRESS, port ), this );
		std::cout << "listening to TUIO/UDP messages on port " << port << std::endl;
	} catch (std::exception &e) {
		std::cerr << "could not bind to UDP port " << port << std::endl;
		socket = NULL;
	}
}

UdpReceiver::~UdpReceiver() {
	disconnect();
	delete socket;
}

void UdpReceiver::connect(bool lk) {

	if (connected) return;
	if (socket==NULL) return;
	locked = lk;
	connected = true;
	thread = 0;
	
	if (!locked) {
		thread_done = false;
#ifndef WIN32
		if (pthread_create(&thread , NULL, ClientThreadFunc, this)!=0)
			thread_done = true;
#else
		DWORD threadId;
		thread = CreateThread( 0, 0, ClientThreadFunc, this, 0, &threadId );
		if (!thread) thread_done = true;
#endif
	} else {
		socket->Run();
		connected = false;
		locked = false;
	}
}

void UdpReceiver::disconnect() {
	
	if (!connected) return;
	if (!locked) {
		// Run() resets the break flag on entry, so a break sent before
		// the thread reached Run() is lost - repeat it until the thread exits
		while (!thread_done) {
			socket->AsynchronousBreak();
#ifndef WIN32
			usleep(1000);
#else
			Sleep(1);
#endif
		}
#ifndef WIN32
		if (thread) pthread_join(thread,NULL);
#else
		if (thread) {
			WaitForSingleObject(thread,INFINITE);
			CloseHandle(thread);
		}
#endif
		thread = 0;
	} else {
		socket->AsynchronousBreak();
		locked = false;
	}

	connected = false;
}


