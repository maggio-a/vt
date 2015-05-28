// =============================================================================
//
//  This file is part of the final project source code for the course "Ad hoc
//  and sensor networks" (Master's degree in Computer Science, University of
//  Pisa)
//
//  Copyright (C) 2015, Andrea Maggiordomo
//
// =============================================================================

#include "Socket.hpp"

#include <string>
#include <errno.h>
#include <cstdio>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>

#include "Msg.hpp"

using namespace std;

namespace rhs {

socketHandle_t CreateSocket(std::string address, int port) {
	return socketHandle_t(new Socket(address, port));
}

Socket::Socket(string addr, int p) : fd(-1), closed(true), err(0) {
	struct sockaddr_in addr_info;

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd < 0) {
		perror("socket");
		throw err = errno;
	}
	addr_info.sin_family = AF_INET;
	addr_info.sin_port = p;
	if (inet_pton(AF_INET, addr.c_str(), &addr_info.sin_addr) == 0) {
		throw "Socket: invalid address";
	}
	if (connect(fd, (struct sockaddr *) &addr_info, sizeof(addr_info))) {
		perror("connect");
		throw err = errno;
	}
	closed = false;
}

Socket::Socket(int fd_) : fd(fd_), closed(false), err(0) { }

Socket::~Socket() {
	if (!closed) {
		try {
			Close();
		} catch (...) {
			cerr << "Failed closing socket" << '\n';
		}
	}
}

Message Socket::Receive(long msec) {
	fd_set readfd;
	FD_ZERO(&readfd);
	FD_SET(fd, &readfd);
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = msec * 1000;
	int n = select(fd+1, &readfd, NULL, NULL, &tv);
	if (n == 0)
		throw Timeout();
	else if (n < 0)
		throw err = errno;
	else
		return Receive();
}

Message Socket::Receive() {
	uint16_t type;
	uint16_t sz;
	Recv(&type, 2);
	Recv(&sz, 2);
	type = ntohs(type);
	sz = ntohs(sz);
	if (sz > 0) {
		char *payload = new char[sz];
		Recv(payload, sz);
		payload[sz-1] = '\0';
		Message m(type, string(payload));
		delete[] payload;
		return m;
	} else {
		return Message(type);
	}
}

void Socket::Recv(void *base, size_t n) {
	ssize_t status = ::recv(fd, base, n, MSG_WAITALL); 
	if (status == 0) {
		throw 0;
	} else if (status < 0) {
		throw err = errno;
	}
}

void Socket::Send(const Message &m) {
	uint16_t type = htons(m.type);
	uint16_t host_sz = m.payload.size();
	uint16_t sz = (host_sz > 0) ? htons(host_sz+1) : htons(0);
	Send(&type, 2);
	Send(&sz, 2);
	if (host_sz > 0)
		Send(m.payload.c_str(), host_sz+1);
}

void Socket::Send(const void *base, size_t n) {
	if (::send(fd, base, n, MSG_NOSIGNAL) < 0) {
		perror("send");
		throw err = errno;
	}
}

void Socket::Close() {
	if (!closed) {
		if (::close(fd)) {
			perror("Socket::Close");
			throw err = errno;
		} else {
			closed = true;
		}
	}
}

} // rhs namespace
