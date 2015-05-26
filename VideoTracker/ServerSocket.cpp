#include "ServerSocket.hpp"

#include <cerrno>
#include <cstdio>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "Socket.hpp"


using namespace std;

namespace rhs {

serverSocketHandle_t CreateServerSocket(int port, int backlog) {
	return serverSocketHandle_t(new ServerSocket(port, backlog));
}

ServerSocket::ServerSocket(int p, int bl) : port(p), backlog(bl), sck(-1), closed(true), err(0) {
	struct sockaddr_in server_info;

	sck = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sck < 0) {
		perror("socket");
		throw err = errno;
	}
	server_info.sin_family = AF_INET;
	server_info.sin_port = port;
	server_info.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sck, (struct sockaddr *) &server_info, sizeof(server_info))) {
		perror("bind");
		throw err = errno;
	}
	if (listen(sck, backlog)) {
		perror("listen");
		throw err = errno;
	}
	closed = false;
}

ServerSocket::~ServerSocket() {
	if (!closed) {
		try {
			Close();
		} catch (...) {
			cerr << "Failed closing server socket" << '\n';
		}
	}
}

socketHandle_t ServerSocket::Accept() {
	struct sockaddr_in incoming_addr;
	socklen_t incoming_addrsize = sizeof(incoming_addr);
	int incoming = accept(sck, (struct sockaddr *) &incoming_addr, &incoming_addrsize);
	if (incoming < 0) {
		perror("ServerSocket::Accept");
		throw err = errno;
	} else {
		return socketHandle_t(new Socket(incoming));
	}
}

void ServerSocket::Close() {
	if (!closed) {
		if (close(sck)) {
			perror("ServerSocket::Close");
			throw err = errno;
		} else {
			closed = true;
		}
	}
}

} // rhs namespace