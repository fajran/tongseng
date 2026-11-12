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

#include "TcpSender.h"
#include "TcpUtils.h"
#include <algorithm>
using namespace TUIO;

namespace {

/** Data passed to a per-client sender thread */
struct TcpSenderClientData {
	TcpSender *sender; ///< the sender owning the client thread
	tuio_socket_t socket; ///< the accepted client socket
};

} // anonymous namespace

TUIO_THREAD_FUNC(ClientThreadFunc)
{
	TcpSenderClientData *client_data = static_cast<TcpSenderClientData*>(obj);
	TcpSender *sender = client_data->sender;
	tuio_socket_t client = client_data->socket;
	delete client_data;
	std::string type = sender->tuio_type();

	// wait until the connection is closed or shut down
	char buf[16];
	while (recv(client, buf, sizeof(buf),0)>0) {}

	lockMutex(sender->tcp_mutex);
	sender->tcp_client_list.remove(client);
	if (client==sender->tcp_socket) sender->tcp_socket = TUIO_INVALID_SOCKET;
	if (sender->tcp_client_list.empty()) sender->connected=false;
	unlockMutex(sender->tcp_mutex);
	closeSocket(client);
	std::cout << type << " connection closed"<< std::endl;

	return 0;
}

TUIO_THREAD_FUNC(ServerThreadFunc)
{
	TcpSender *sender = static_cast<TcpSender*>(obj);
	
	std::cout << sender->tuio_type() << " socket created on port " << sender->port_no << std::endl;
	while (isValidSocket(sender->tcp_socket)) {
		struct sockaddr_in client_addr;
		socklen_t len = sizeof(client_addr);
		tuio_socket_t tcp_client = accept(sender->tcp_socket, (struct sockaddr*)&client_addr, &len);
		if (!isValidSocket(tcp_client)) break;

		// stop on the dummy self-connection used to unblock accept() during teardown
		lockMutex(sender->tcp_mutex);
		bool stop = !isValidSocket(sender->tcp_socket);
		if (!stop) sender->pending_socket = tcp_client;
		unlockMutex(sender->tcp_mutex);
		if (stop) {
			closeSocket(tcp_client);
			break;
		}

		std::cout << sender->tuio_type() << " client connected from " << inet_ntoa(client_addr.sin_addr) << "@" << client_addr.sin_port << std::endl;
		setSocketOptions(tcp_client);

		// complete the protocol handshake before the client receives any data
		sender->newClient((int)tcp_client);

		lockMutex(sender->tcp_mutex);
		sender->pending_socket = TUIO_INVALID_SOCKET;
		sender->tcp_client_list.push_back(tcp_client);
		sender->connected = true;
		unlockMutex(sender->tcp_mutex);

		TcpSenderClientData *client_data = new TcpSenderClientData;
		client_data->sender = sender;
		client_data->socket = tcp_client;
		if (!startDetachedThread(ClientThreadFunc, client_data)) {
			delete client_data;
			lockMutex(sender->tcp_mutex);
			sender->tcp_client_list.remove(tcp_client);
			if (sender->tcp_client_list.empty()) sender->connected = false;
			unlockMutex(sender->tcp_mutex);
			closeSocket(tcp_client);
		}
	}
	
	return 0;
}

TcpSender::TcpSender()
	:connected (false)
{
	local = true;
	buffer_size = MAX_TCP_SIZE;
	port_no = 3333;
	server_thread = 0;
	pending_socket = TUIO_INVALID_SOCKET;
	initMutex(tcp_mutex);

	// the default sender fails silently if no receiver is available
	connectTo("127.0.0.1",3333);
}

TcpSender::TcpSender(const char *host, int port) 
	:connected (false)
{	
	local = ((strcmp(host,"127.0.0.1")==0) || (strcmp(host,"localhost")==0));
	buffer_size = MAX_TCP_SIZE;
	port_no = port;
	server_thread = 0;
	pending_socket = TUIO_INVALID_SOCKET;
	initMutex(tcp_mutex);

	if (!connectTo(host,port)) {
		destroyMutex(tcp_mutex);
		throw std::exception();
	}
}

TcpSender::TcpSender(int port)
	:connected (false)
{
	local = false;
	buffer_size = MAX_TCP_SIZE;
	port_no = port;
	server_thread = 0;
	pending_socket = TUIO_INVALID_SOCKET;
	initMutex(tcp_mutex);
	
	tcp_socket = openListenSocket(port);
	if (!isValidSocket(tcp_socket)) {
		destroyMutex(tcp_mutex);
		throw std::exception();
	}

	startThread(server_thread, ServerThreadFunc, this);
}

bool TcpSender::connectTo(const char *host, int port) {

	tcp_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if (!isValidSocket(tcp_socket)) {
		std::cerr << "could not create " << tuio_type() << " socket" << std::endl;
		return false;
	}
	setSocketOptions(tcp_socket);

	struct sockaddr_in tcp_server;
	if (!resolveHost(host,port,tcp_server)) {
		std::cerr << "unknown host name: " << host << std::endl;
		closeSocket(tcp_socket);
		tcp_socket = TUIO_INVALID_SOCKET;
		return false;
	}

	if (::connect(tcp_socket,(struct sockaddr*)&tcp_server,sizeof(tcp_server))<0) {
		std::cerr << "could not open " << tuio_type() << " connection to " << host << ":"<< port << std::endl;
		closeSocket(tcp_socket);
		tcp_socket = TUIO_INVALID_SOCKET;
		return false;
	}

	std::cout << tuio_type() << " connection opened to " << host << ":"<< port << std::endl;
	tcp_client_list.push_back(tcp_socket);
	connected = true;

	// the client thread owns and closes the connected socket
	TcpSenderClientData *client_data = new TcpSenderClientData;
	client_data->sender = this;
	client_data->socket = tcp_socket;
	if (!startThread(server_thread, ClientThreadFunc, client_data)) {
		delete client_data;
		tcp_client_list.clear();
		connected = false;
		closeSocket(tcp_socket);
		tcp_socket = TUIO_INVALID_SOCKET;
		return false;
	}

	return true;
}

bool TcpSender::isConnected() {
	return connected;
}

TcpSender::~TcpSender() {

	// shut down all client sockets to unblock their threads, which then close them,
	// only a listening socket that is not owned by a client thread is closed here
	tuio_socket_t listen_socket = TUIO_INVALID_SOCKET;
	lockMutex(tcp_mutex);
	for (std::list<tuio_socket_t>::iterator client = tcp_client_list.begin(); client!=tcp_client_list.end(); client++)
		shutdownSocket(*client);
	if (isValidSocket(pending_socket)) shutdownSocket(pending_socket);
	if (isValidSocket(tcp_socket) && (std::find(tcp_client_list.begin(),tcp_client_list.end(),tcp_socket)==tcp_client_list.end()))
		listen_socket = tcp_socket;
	tcp_socket = TUIO_INVALID_SOCKET;
	unlockMutex(tcp_mutex);

	if (isValidSocket(listen_socket)) {
		wakeAccept(listen_socket);
		closeSocket(listen_socket);
	}

	joinThread(server_thread);
	waitForEmptyList(tcp_mutex,tcp_client_list);
	destroyMutex(tcp_mutex);
}

bool TcpSender::sendOscPacket (osc::OutboundPacketStream *bundle) {
	if (!connected) return false; 
	if ( bundle->Size() > buffer_size ) return false;
	if ( bundle->Size() == 0 ) return false;
	
	// the OSC packet is prefixed with its size as a 32-bit big-endian integer
	int size = (int)bundle->Size();
	data_size[0] = (char)((size>>24) & 0xFF);
	data_size[1] = (char)((size>>16) & 0xFF);
	data_size[2] = (char)((size>>8) & 0xFF);
	data_size[3] = (char)(size & 0xFF);

	return deliver(data_size,4,bundle->Data(),size);
}

bool TcpSender::deliver(const char *header, int header_size, const char *data, int data_len) {

	lockMutex(tcp_mutex);

	// combine header and payload into a single write where possible
	bool combined = (header_size+data_len <= (int)sizeof(data_buffer));
	if (combined) {
		memmove(&data_buffer[0], header, header_size);
		memcpy(&data_buffer[header_size], data, data_len);
	}

	for (std::list<tuio_socket_t>::iterator client = tcp_client_list.begin(); client!=tcp_client_list.end(); client++) {
		bool sent = combined ? sendAll((*client),data_buffer,header_size+data_len)
			: (sendAll((*client),header,header_size) && sendAll((*client),data,data_len));
		// shut down the socket so that the client thread removes it
		if (!sent) shutdownSocket(*client);
	}

	unlockMutex(tcp_mutex);
	return true;
}

void TcpSender::newClient( int ) { }
