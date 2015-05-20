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

#include "msg.hpp"

using namespace std;
using namespace rhs;

Socket::Socket(string addr, int p) : _err(0) {
	struct sockaddr_in addr_info;
	int r;

	_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_fd < 0) {
		perror("socket");
		throw _err = errno;
	}
	addr_info.sin_family = AF_INET;
	addr_info.sin_port = p;
	if (inet_pton(AF_INET, addr.c_str(), &addr_info.sin_addr) == 0) {
		throw "Socket: invalid address";
	}
	if (connect(_fd, (struct sockaddr *) &addr_info, sizeof(addr_info))) {
		perror("connect");
		throw _err = errno;
	}
	_closed = false;
}

Socket::Socket(int fd) : _fd(fd), _closed(false), _err(0) { }

Socket::~Socket() {
	if (!_closed) {
		try {
			close();
		} catch (...) {
			cerr << "Failed closing socket" << '\n';
			perror("close");
		}
	}
}

Message Socket::Receive(long msec) {
	fd_set readfd;
	FD_ZERO(&readfd);
	FD_SET(_fd, &readfd);
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = msec * 1000;
	int n = select(_fd+1, &readfd, NULL, NULL, &tv);
	if (n == 0)
		throw Timeout();
	else if (n < 0)
		throw _err = errno;
	else
		return Receive();
}

Message Socket::Receive() {
	rhs::message_t type;
	uint16_t sz;
	recv(&type, 2);
	recv (&sz, 2);
	type = ntohs(type);
	sz = ntohs(sz);
	if (sz > 0) {
		char *payload = new char[sz];
		recv(payload, sz);
		payload[sz-1] = '\0';
		Message m(type, string(payload));
		delete[] payload;
		return m;
	} else {
		return Message(type);
	}
}

// FIXME needs integrity checks
void Socket::recv(void *base, size_t n) {
	ssize_t status = ::recv(_fd, base, n, MSG_WAITALL); 
	if (status == 0) {
		throw 0;
	} else if (status < 0) {
		throw _err = errno;
	}
}

void Socket::Send(const Message &m) {
	rhs::message_t type = htons(m.type);
	uint16_t host_sz = m.payload.size();
	uint16_t sz = (host_sz > 0) ? htons(host_sz+1) : htons(0);
	send(&type, 2);
	send(&sz, 2);
	if (host_sz > 0)
		send(m.payload.c_str(), host_sz+1);
}

// FIXME needs integrity checks
void Socket::send(const void *base, size_t n) {
	if (::send(_fd, base, n, MSG_NOSIGNAL) < 0) {
		perror("send");
		throw _err = errno;
	}
}

void Socket::close() {
	if (!_closed) {
		if (::close(_fd)) {
			perror("close");
			throw _err = errno;
		} else {
			_closed = true;
		}
	}
}