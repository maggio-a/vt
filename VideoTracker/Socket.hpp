// =============================================================================
//
//  This file is part of the final project source code for the course "Ad hoc
//  and sensor networks" (Master's degree in Computer Science, University of
//  Pisa)
//
//  Copyright (C) 2015, Andrea Maggiordomo
//
// =============================================================================

#ifndef RHS_SOCKET_HDR
#define RHS_SOCKET_HDR

#include <string>
#include <memory>


namespace rhs {

const int SERVER_PORT_DEFAULT = 12345;

class ServerSocket;
class Socket;
class Message;

typedef std::shared_ptr<Socket> socketHandle_t;


// function CreateSocket
// Factory function. Returns a smart ptr to a Socket connected to the specified address (IPV4)
// and port, or throws the errno code if any of the underlying socket API calls failed
socketHandle_t CreateSocket(std::string address, int port);

// class Socket
// Simple abstraction built on the berkeley sockets API, provides functionality to send and receive
// rhs::Message structures and raw data. Constructors are private, an instance of a Socket connected
// to a specified endpoint can be obtained through a call to the factory function CreateSocket.
// EXCEPTIONS: If any error occurs durning a method call, the corresponding errno code is thrown (fixme :-)
class Socket {
public:
	// thrown by Socket::Receive(long) to notify the caller that the timeout was triggered
	struct Timeout { };

	~Socket();

	// Reads n bytes of raw data from the socket and writes them to the buffer pointed by base
	void Recv(void *base, size_t n);
	// Writes n bytes of raw data to the socket from the buffer pointed by base
	void Send(const void *base, size_t n);

	// Sends a rhs::Message through the socket (blocks until all the data has been sent)
	void Send(const rhs::Message &msg);
	// Receives a rhs::Message from the socket. Blocking call
	rhs::Message Receive();
	// Receives a rhs::Message from the socket. If after msec milliseconds no data is available,
	// throws a Socket::Timeout object
	rhs::Message Receive(long msec);

	// Closes the socket
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

} // rhs namespace

#endif
