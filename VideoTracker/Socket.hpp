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

class ServerSocket;

class Socket {
public:
	Socket(std::string address, int port);
	~Socket();

	void recv(void *base, size_t n);
	void send(void *base, size_t n);
	void close();
	
private:
	//friend Socket ServerSocket::accept();
	friend class ServerSocket;
	Socket(int fd); // used by ServerSocket on accept() return

	int _fd;
	int _err; // remove this
	bool _closed;
};

Socket::Socket(std::string addr, int p) {
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

Socket::Socket(int fd) : _fd(fd) { }

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

// FIXME needs integrity checks
void Socket::recv(void *base, size_t n) {
	ssize_t status = ::recv(_fd, base, n, MSG_WAITALL); 
	if (status == 0) {
		throw 0;
	} else if (status < 0) {
		throw _err = errno;
	}
}

// FIXME needs integrity checks
void Socket::send(void *base, size_t n) {
	if (::send(_fd, base, n, 0) < 0) {
		perror("send");
		throw _err = errno;
	}
}

void Socket::close() {
	if (::close(_fd)) {
		perror("close");
		throw _err = errno;
	}
}

#endif