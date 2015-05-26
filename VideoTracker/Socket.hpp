#ifndef RHS_SOCKET_HDR
#define RHS_SOCKET_HDR

#include <string>
#include <memory>
#include "msg.hpp"


namespace rhs {
	const int SERVER_PORT_DEFAULT = 12345;
}

class ServerSocket;
class Socket;

typedef std::shared_ptr<Socket> socketHandle_t;

// factory function
socketHandle_t CreateSocket(std::string address, int port);

class Socket {
public:
	struct Timeout { };

	~Socket();

	void Recv(void *base, size_t n);
	void Send(const void *base, size_t n);

	void Send(const rhs::Message &msg);
	rhs::Message Receive();
	rhs::Message Receive(long msec);

	void Close();
	
private:
	friend class ServerSocket;
	friend socketHandle_t CreateSocket(std::string address, int port);

	Socket(std::string address, int port);
	Socket(int fd); // used by ServerSocket::Accept() on return

	int fd;
	bool closed;
	int err; // remove this
};

#endif