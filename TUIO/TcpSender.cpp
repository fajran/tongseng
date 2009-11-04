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


#include "TcpSender.h"

#ifdef  WIN32
#ifndef int32_t
typedef DWORD int32_t;
#endif
#endif

using namespace TUIO;

#ifndef  WIN32
static void* ServerThreadFunc( void* obj )
#else
static DWORD WINAPI ServerThreadFunc( LPVOID obj )
#endif
{
	TcpSender *sender = static_cast<TcpSender*>(obj);
	struct sockaddr_in tcp_client;
	socklen_t len = sizeof(tcp_client);
	char buf[16];
	
	while (sender->tcp_socket) {
		sender->tcp_client_connection = accept(sender->tcp_socket, (struct sockaddr*)&tcp_client, &len);
		sender->connected=true;
		
		// test if connection is still alive
		int connected = 1;
		while (connected) {
			connected = recv(sender->tcp_client_connection, buf, sizeof(buf),0);
		}
		sender->connected=false;
	}
	
	return 0;
};

TcpSender::TcpSender()
	:connected (false) 
{
	local = true;
	
	tcp_socket = socket( AF_INET, SOCK_STREAM, 0 );
	if (tcp_socket < 0) std::cerr << "could not create socket" << std::endl;
	
	struct sockaddr_in tcp_server;
	memset( &tcp_server, 0, sizeof (tcp_server));
	unsigned long addr = inet_addr("127.0.0.1");
	memcpy( (char *)&tcp_server.sin_addr, &addr, sizeof(addr));
	
	tcp_server.sin_family = AF_INET;
	tcp_server.sin_port = htons(3333);
	
	int ret = connect(tcp_client_connection,(struct sockaddr*)&tcp_server,sizeof(tcp_server));
	if (ret<0)	std::cerr << "could not connect to server" << std::endl;
	
	buffer_size = MAX_TCP_SIZE;
}

TcpSender::TcpSender(const char *host, int port) 
	:connected (false) 
{	
	if ((strcmp(host,"127.0.0.1")==0) || (strcmp(host,"localhost")==0)) {
		local = true;
	} else local = false;
	
	tcp_socket = socket( AF_INET, SOCK_STREAM, 0 );
	if (tcp_socket < 0) std::cerr << "could not create socket" << std::endl;

	struct sockaddr_in tcp_server;
	memset( &tcp_server, 0, sizeof (tcp_server));
	unsigned long addr = inet_addr(host);
	if (addr != INADDR_NONE) {
		memcpy( (char *)&tcp_server.sin_addr, &addr, sizeof(addr));
	} else {
		struct hostent *host_info = gethostbyname(host);
		if (host_info == NULL) std::cerr << "unknown host name" << std::endl;
		memcpy( (char *)&tcp_server.sin_addr, host_info->h_addr, host_info->h_length );
	}

	tcp_server.sin_family = AF_INET;
	tcp_server.sin_port = htons(port);

	int ret = connect(tcp_client_connection,(struct sockaddr*)&tcp_server,sizeof(tcp_server));
	if (ret<0)	std::cerr << "could not connect to server" << std::endl;
		
	buffer_size = MAX_TCP_SIZE;
}

TcpSender::TcpSender(int port)
	:connected (false) 
{
	local = false;
	
	tcp_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (tcp_socket < 0) std::cerr << "could not create socket" << std::endl;

	int optval = 1;
	#ifdef  WIN32
	int ret = setsockopt(tcp_socket,SOL_SOCKET,SO_REUSEADDR, (const char *)&optval,  sizeof(int));
	#else
	int ret = setsockopt(tcp_socket,SOL_SOCKET,SO_REUSEADDR, (const void *)&optval,  sizeof(int));
	#endif
	if (ret < 0) std::cerr << "could not reuse socket address" << std::endl;
	
	struct sockaddr_in tcp_server;
	memset( &tcp_server, 0, sizeof (tcp_server));

	tcp_server.sin_family = AF_INET;
	tcp_server.sin_addr.s_addr = htonl(INADDR_ANY);
	tcp_server.sin_port = htons(port);

	socklen_t len = sizeof(tcp_server);
	ret = bind(tcp_socket,(struct sockaddr*)&tcp_server,len);
	if (ret < 0) std::cerr << "could not bind to socket" << std::endl;
	
	ret =  listen(tcp_socket, 1);
	if (ret < 0) std::cerr << "could not listen to socket" << std::endl;
				
#ifndef WIN32
	pthread_create(&server_thread , NULL, ServerThreadFunc, this);
#else
	DWORD threadId;
	server_thread = CreateThread( 0, 0, ServerThreadFunc, this, 0, &threadId );
#endif
	
	buffer_size = MAX_TCP_SIZE;
}

TcpSender::~TcpSender() {
#ifdef WIN32
	closesocket(tcp_client_connection);
	closesocket(tcp_socket);
	if( server_thread ) CloseHandle( server_thread );
#else
	close(tcp_client_connection);
	close(tcp_socket);
	server_thread = 0;
#endif		
}


bool TcpSender::isConnected() { 
	return connected;
}

bool TcpSender::sendOscPacket (osc::OutboundPacketStream *bundle) {
	if (!connected) return false; 
	if ( bundle->Size() > buffer_size ) return false;
	
#ifdef OSC_HOST_LITTLE_ENDIAN             
	data_size[0] = bundle->Size()>>24;
	data_size[1] = (bundle->Size()>>16) & 255;
	data_size[2] = (bundle->Size()>>8) & 255;
	data_size[3] = (bundle->Size()) & 255;
#else
	*((int32_t*)data_size) = bundle->Size();
#endif

	send(tcp_client_connection,data_size, 4,0);
	send(tcp_client_connection, bundle->Data(), bundle->Size(),0); 

	return true;
}
