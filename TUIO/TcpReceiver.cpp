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

#include "TcpReceiver.h"
#include "TcpUtils.h"

using namespace TUIO;
using namespace osc;

namespace {

/** Data passed to a per-client receiver thread */
struct TcpClientData {
	TcpReceiver *receiver; ///< the receiver owning the client thread
	tuio_socket_t socket; ///< the accepted client socket
};

} // anonymous namespace

TUIO_THREAD_FUNC(ClientThreadFunc)
{
	TcpClientData *client_data = static_cast<TcpClientData*>(obj);
	TcpReceiver *receiver = client_data->receiver;
	tuio_socket_t client = client_data->socket;
	delete client_data;

	char data_buffer[MAX_TCP_SIZE+4];
	int data_len = 0;

	int32_t bytes = 1;
	while (bytes>0) {
		bytes = (int32_t)recv(client, data_buffer+data_len, (int)sizeof(data_buffer)-data_len,0);
		if (bytes<=0) break;
		data_len += bytes;
		
		// extract all complete OSC frames from the received TCP stream
		int offset = 0;
		while ((data_len-offset)>=4) {
			
			// the frame size is a 32-bit big-endian integer
			int32_t bundle_size = (int32_t)(0xFF & data_buffer[offset]) << 24;
			bundle_size |= (0xFF & data_buffer[offset+1]) << 16;
			bundle_size |= (0xFF & data_buffer[offset+2]) << 8;
			bundle_size |= (0xFF & data_buffer[offset+3]);
			
			// abort the connection on a corrupt frame size
			if ((bundle_size<0) || (bundle_size>MAX_TCP_SIZE)) {
				offset = data_len;
				data_len = 0;
				bytes = 0;
				break;
			}
			
			// wait for more data on an incomplete frame
			if ((data_len-offset)<(4+bundle_size)) break;
			
			receiver->ProcessPacket(&data_buffer[offset+4],(int)bundle_size,IpEndpointName());
			offset += 4+bundle_size;
		}
		
		// compact the remaining data to the front of the buffer
		if (offset>0) {
			memmove(data_buffer,data_buffer+offset,data_len-offset);
			data_len -= offset;
		}
	}

	// the client thread owns and closes its socket, including the connected main socket
	lockMutex(receiver->tcp_mutex);
	receiver->tcp_client_list.remove(client);
	if (client==receiver->tcp_socket) receiver->tcp_socket = TUIO_INVALID_SOCKET;
	unlockMutex(receiver->tcp_mutex);
	closeSocket(client);
	std::cout << "closed TUIO/TCP connection" << std::endl;

	return 0;
}

TUIO_THREAD_FUNC(ServerThreadFunc)
{
	TcpReceiver *receiver = static_cast<TcpReceiver*>(obj);

	while (isValidSocket(receiver->tcp_socket)) {
		struct sockaddr_in client_addr;
		socklen_t len = sizeof(client_addr);
		tuio_socket_t tcp_client = accept(receiver->tcp_socket, (struct sockaddr*)&client_addr, &len);
		if (!isValidSocket(tcp_client)) break;

		// stop on the dummy self-connection used to unblock accept() during disconnect()
		lockMutex(receiver->tcp_mutex);
		bool stop = !receiver->isConnected();
		if (!stop) receiver->tcp_client_list.push_back(tcp_client);
		unlockMutex(receiver->tcp_mutex);
		if (stop) {
			closeSocket(tcp_client);
			break;
		}

		std::cout << "listening to TUIO/TCP messages from " << inet_ntoa(client_addr.sin_addr) << "@" << client_addr.sin_port << std::endl;

		TcpClientData *client_data = new TcpClientData;
		client_data->receiver = receiver;
		client_data->socket = tcp_client;
		if (!startDetachedThread(ClientThreadFunc, client_data)) {
			delete client_data;
			lockMutex(receiver->tcp_mutex);
			receiver->tcp_client_list.remove(tcp_client);
			unlockMutex(receiver->tcp_mutex);
			closeSocket(tcp_client);
		}
	}
	
	return 0;
}

TcpReceiver::TcpReceiver(int port)
: tcp_socket	(TUIO_INVALID_SOCKET)
, server_thread	(0)
, locked	(false)
, local		(true)
{
	initMutex(tcp_mutex);

	tcp_socket = openListenSocket(port);
	if (isValidSocket(tcp_socket)) std::cout << "TUIO/TCP socket created on port " << port << std::endl;
}

TcpReceiver::TcpReceiver(const char *host, int port)
: tcp_socket	(TUIO_INVALID_SOCKET)
, server_thread	(0)
, locked		(false)
, local			(false)
{
	initMutex(tcp_mutex);

	struct sockaddr_in tcp_server;
	if (!resolveHost(host,port,tcp_server)) {
		std::cerr << "unknown host name: " << host << std::endl;
		return;
	}

	tcp_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (!isValidSocket(tcp_socket)) {
		std::cerr << "could not create TUIO/TCP socket" << std::endl;
		return;
	}

	if (::connect(tcp_socket,(struct sockaddr*)&tcp_server,sizeof(tcp_server))<0) {
		std::cerr << "could not connect to TUIO/TCP server at " << host << ":"<< port << std::endl;
		closeSocket(tcp_socket);
		tcp_socket = TUIO_INVALID_SOCKET;
		return;
	}

	tcp_client_list.push_back(tcp_socket);
	std::cout << "listening to TUIO/TCP messages from " << host << ":" << port << std::endl;
}

TcpReceiver::~TcpReceiver() {
	disconnect();
	
	// close a socket that has never been handed over to a receiver thread
	if (isValidSocket(tcp_socket)) closeSocket(tcp_socket);
	destroyMutex(tcp_mutex);
}

void TcpReceiver::connect(bool lk) {
	
	if (connected) return;
	if (!isValidSocket(tcp_socket)) return;
	locked = lk;
	connected = true;
	server_thread = 0;

	if (!local) {
		// the connected main socket is handled by a single client thread
		TcpClientData *client_data = new TcpClientData;
		client_data->receiver = this;
		client_data->socket = tcp_socket;
		
		if (locked) {
			ClientThreadFunc(client_data);
			connected = false;
			locked = false;
		} else if (!startThread(server_thread, ClientThreadFunc, client_data)) {
			delete client_data;
			connected = false;
		}
	} else {
		if (locked) {
			ServerThreadFunc(this);
			connected = false;
			locked = false;
		} else if (!startThread(server_thread, ServerThreadFunc, this)) {
			connected = false;
		}
	}
}

void TcpReceiver::disconnect() {
	
	if (!connected) return;

	// shut down all client sockets to unblock their threads, which then close them
	tuio_socket_t listen_socket = TUIO_INVALID_SOCKET;
	lockMutex(tcp_mutex);
	connected = false;
	for (std::list<tuio_socket_t>::iterator client = tcp_client_list.begin(); client!=tcp_client_list.end(); client++)
		shutdownSocket(*client);
	if (local) {
		listen_socket = tcp_socket;
		tcp_socket = TUIO_INVALID_SOCKET;
	}
	unlockMutex(tcp_mutex);

	if (isValidSocket(listen_socket)) {
		wakeAccept(listen_socket);
		closeSocket(listen_socket);
	}

	if (!locked) joinThread(server_thread);
	else locked = false;

	waitForEmptyList(tcp_mutex,tcp_client_list);
}
