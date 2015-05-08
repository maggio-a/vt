#ifndef RHS_SERVERSOCKET_HDR
#define RHS_SERVERSOCKET_HDR

#include <string>
#include <errno.h>
#include <cstdio>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "Socket.hpp"

class ServerSocket {
public:
	ServerSocket(int port, int backlog);
	~ServerSocket();

	Socket * accept();
	void close();
	
private:
	int _port;
	int _backlog;
	int _sck;
	int _err;
	bool _closed;
};

ServerSocket::ServerSocket(int p, int bl)
		: _port(p), _backlog(bl) {
 	struct sockaddr_in server_info;

	_sck = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_sck < 0) {
		perror("socket");
		throw _err = errno;
	}
	server_info.sin_family = AF_INET;
	server_info.sin_port = _port;
	server_info.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(_sck, (struct sockaddr *) &server_info, sizeof(server_info))) {
		perror("bind");
		throw _err = errno;
	}
	if (listen(_sck, _backlog)) {
		perror("listen");
		throw _err = errno;
	}
	_closed = false;
}

ServerSocket::~ServerSocket() {
	if (!_closed) {
		try {
			close();
		} catch (...) {
			std::cerr << "Failed closing server socket" << std::endl;
			perror("close");
		}
	}
}

Socket * ServerSocket::accept() {
	struct sockaddr_in incoming_addr;
	socklen_t incoming_addrsize = sizeof(incoming_addr);
	int incoming = ::accept(_sck, (struct sockaddr *) &incoming_addr, &incoming_addrsize);
	if (incoming < 0) {
		perror("accept");
		throw _err = errno;
	} else {
		//Socket s(incoming);
		//return s;
		return new Socket(incoming);
	}
}

void ServerSocket::close() {
	if (!_closed) {
		if (::close(_sck)) {
			perror("close");
			throw _err = errno;
		} else {
			_closed = true;
		}
	}
}

#endif