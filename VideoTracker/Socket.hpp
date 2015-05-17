#ifndef RHS_SOCKET_HDR
#define RHS_SOCKET_HDR

#include <string>
#include "msg.hpp"


namespace rhs {
	const int SERVER_PORT_DEFAULT = 12345;
}

class ServerSocket;


class Socket {
public:
	struct Timeout { };

	Socket(std::string address, int port);
	~Socket();

	void recv(void *base, size_t n);
	void send(const void *base, size_t n);

	void Send(const rhs::Message &msg);
	rhs::Message Receive();
	rhs::Message Receive(long msec);

	void close();
	
private:
	//friend Socket ServerSocket::accept();
	friend class ServerSocket;
	Socket(int fd); // used by ServerSocket on accept() return

	int _fd;
	bool _closed;
	int _err; // remove this
};

#endif