#ifndef RHS_SOCKET_HDR
#define RHS_SOCKET_HDR

#include <errno.h>
#include <cstdio>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "msg.hpp"

class ServerSocket;

class Socket {
public:
	Socket(std::string address, int port);
	~Socket();

	void recv(void *base, size_t n);
	void send(const void *base, size_t n);

	void Send(const rhs::Message &msg);
	rhs::Message Receive();

	void close();
	
private:
	//friend Socket ServerSocket::accept();
	friend class ServerSocket;
	Socket(int fd); // used by ServerSocket on accept() return

	int _fd;
	bool _closed;
	int _err; // remove this
};

Socket::Socket(std::string addr, int p) : _err(0) {
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
			std::cerr << "Failed closing socket" << std::endl;
			perror("close");
		}
	}
}

rhs::Message Socket::Receive() {
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
		rhs::Message m(type, std::string(payload));
		delete[] payload;
		return m;
	} else {
		return rhs::Message(type);
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

void Socket::Send(const rhs::Message &m) {
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
	if (::send(_fd, base, n, 0) < 0) {
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

#endif