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

/*
 Internal platform helpers shared by the TCP transport implementations.
 This header is not part of the public API and must be included after
 TcpSender.h or TcpReceiver.h, which provide the socket headers.
*/

#ifndef INCLUDED_TCPUTILS_H
#define INCLUDED_TCPUTILS_H

#ifndef WIN32
#include <netinet/tcp.h>
#include <pthread.h>
#endif
#include <string.h>
#include <iostream>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#if defined(WIN32) && !defined(SD_BOTH)
#define SD_BOTH 2
#endif

#ifdef WIN32
#define TUIO_INVALID_SOCKET INVALID_SOCKET
#define TUIO_THREAD_FUNC(name) static DWORD WINAPI name( LPVOID obj )
#else
#define TUIO_INVALID_SOCKET -1
#define TUIO_THREAD_FUNC(name) static void* name( void* obj )
#endif

namespace TUIO {

#ifdef WIN32
	typedef SOCKET tuio_socket_t;
	typedef HANDLE tuio_mutex_t;
	typedef HANDLE tuio_thread_t;
	typedef LPTHREAD_START_ROUTINE tuio_thread_func_t;
#else
	typedef int tuio_socket_t;
	typedef pthread_mutex_t tuio_mutex_t;
	typedef pthread_t tuio_thread_t;
	typedef void* (*tuio_thread_func_t)(void*);
#endif

	static inline bool isValidSocket(tuio_socket_t s) {
#ifdef WIN32
		return s!=INVALID_SOCKET;
#else
		return s>=0;
#endif
	}

	static inline void closeSocket(tuio_socket_t s) {
#ifdef WIN32
		closesocket(s);
#else
		close(s);
#endif
	}

	// unblocks any thread waiting in recv() or send() on the socket
	static inline void shutdownSocket(tuio_socket_t s) {
#ifdef WIN32
		shutdown(s,SD_BOTH);
#else
		shutdown(s,SHUT_RDWR);
#endif
	}

	static inline void initMutex(tuio_mutex_t &m) {
#ifdef WIN32
		m = CreateMutex(NULL,FALSE,NULL);
#else
		pthread_mutex_init(&m,NULL);
#endif
	}

	static inline void lockMutex(tuio_mutex_t &m) {
#ifdef WIN32
		WaitForSingleObject(m,INFINITE);
#else
		pthread_mutex_lock(&m);
#endif
	}

	static inline void unlockMutex(tuio_mutex_t &m) {
#ifdef WIN32
		ReleaseMutex(m);
#else
		pthread_mutex_unlock(&m);
#endif
	}

	static inline void destroyMutex(tuio_mutex_t &m) {
#ifdef WIN32
		if (m) CloseHandle(m);
		m = NULL;
#else
		pthread_mutex_destroy(&m);
#endif
	}

	static inline void sleepMs(int ms) {
#ifdef WIN32
		Sleep(ms);
#else
		usleep(ms*1000);
#endif
	}

	// starts a joinable thread, the handle is zero if the thread could not be created
	static inline bool startThread(tuio_thread_t &t, tuio_thread_func_t func, void *arg) {
#ifdef WIN32
		DWORD threadId;
		t = CreateThread(0,0,func,arg,0,&threadId);
		return (t!=NULL);
#else
		if (pthread_create(&t,NULL,func,arg)==0) return true;
		t = 0;
		return false;
#endif
	}

	// starts a thread that is never joined and releases its resources on exit
	static inline bool startDetachedThread(tuio_thread_func_t func, void *arg) {
		tuio_thread_t t;
		if (!startThread(t,func,arg)) return false;
#ifdef WIN32
		CloseHandle(t);
#else
		pthread_detach(t);
#endif
		return true;
	}

	static inline void joinThread(tuio_thread_t &t) {
		if (!t) return;
#ifdef WIN32
		WaitForSingleObject(t,INFINITE);
		CloseHandle(t);
#else
		pthread_join(t,NULL);
#endif
		t = 0;
	}

	// waits until all client threads have removed themselves from the list
	template<typename L>
	static inline void waitForEmptyList(tuio_mutex_t &m, L &list) {
		for (;;) {
			lockMutex(m);
			bool empty = list.empty();
			unlockMutex(m);
			if (empty) return;
			sleepMs(1);
		}
	}

	// disables SIGPIPE where supported and Nagle's algorithm for low latency delivery
	static inline void setSocketOptions(tuio_socket_t s) {
		int optval = 1;
#ifdef SO_NOSIGPIPE
		setsockopt(s,SOL_SOCKET,SO_NOSIGPIPE,(const char*)&optval,sizeof(optval));
#endif
		setsockopt(s,IPPROTO_TCP,TCP_NODELAY,(const char*)&optval,sizeof(optval));
	}

	// keeps sending until all data has been written
	static inline bool sendAll(tuio_socket_t s, const char *data, int len) {
		int sent_bytes = 0;
		while (sent_bytes<len) {
			int bytes = (int)send(s,data+sent_bytes,len-sent_bytes,MSG_NOSIGNAL);
			if (bytes<=0) return false;
			sent_bytes += bytes;
		}
		return true;
	}

	// resolves a dotted IP address or host name into a socket address
	static inline bool resolveHost(const char *host, int port, struct sockaddr_in &addr) {
		memset(&addr,0,sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = inet_addr(host);
		if (addr.sin_addr.s_addr!=INADDR_NONE) {
			return true;
		}
		struct hostent *host_info = gethostbyname(host);
		if (host_info==NULL) return false;
		memcpy(&addr.sin_addr,host_info->h_addr,host_info->h_length);
		return true;
	}

	// creates a socket listening on the provided port, or an invalid socket on failure
	static inline tuio_socket_t openListenSocket(int port) {
		tuio_socket_t s = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		if (!isValidSocket(s)) {
			std::cerr << "could not create TUIO/TCP socket" << std::endl;
			return TUIO_INVALID_SOCKET;
		}

		int optval = 1;
		if (setsockopt(s,SOL_SOCKET,SO_REUSEADDR,(const char*)&optval,sizeof(optval))<0) {
			std::cerr << "could not reuse TUIO/TCP socket address" << std::endl;
			closeSocket(s);
			return TUIO_INVALID_SOCKET;
		}

		struct sockaddr_in addr;
		memset(&addr,0,sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_port = htons(port);
		if (bind(s,(struct sockaddr*)&addr,sizeof(addr))<0) {
			std::cerr << "could not bind to TUIO/TCP socket on port " << port << std::endl;
			closeSocket(s);
			return TUIO_INVALID_SOCKET;
		}

		if (listen(s,1)<0) {
			std::cerr << "could not start listening to TUIO/TCP socket" << std::endl;
			closeSocket(s);
			return TUIO_INVALID_SOCKET;
		}

		return s;
	}

	// shutdown() and close() do not unblock accept() on all platforms,
	// so a dummy connection to the listening socket makes it return
	static inline void wakeAccept(tuio_socket_t listen_socket) {
		struct sockaddr_in self;
		socklen_t len = sizeof(self);
		if (getsockname(listen_socket,(struct sockaddr*)&self,&len)!=0) return;
		tuio_socket_t s = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		if (!isValidSocket(s)) return;
		self.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		::connect(s,(struct sockaddr*)&self,sizeof(self));
		closeSocket(s);
	}
}

#endif /* INCLUDED_TCPUTILS_H */
