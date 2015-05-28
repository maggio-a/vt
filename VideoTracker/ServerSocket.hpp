// =============================================================================
//
//  This file is part of the final project source code for the course "Ad hoc
//  and sensor networks" (Master's degree in Computer Science, University of
//  Pisa)
//
//  Copyright (C) 2015, Andrea Maggiordomo
//
// =============================================================================

#ifndef RHS_SERVERSOCKET_HDR
#define RHS_SERVERSOCKET_HDR

#include <memory>
#include "Socket.hpp"

namespace rhs {

class ServerSocket;
typedef std::unique_ptr<ServerSocket> serverSocketHandle_t;

// function CreateServerSocket
// Factory function. Returns a smart (unique_)ptr to a ServerSocket listening to the specified port
// with the specified backlog size, or throws the errno code if any of the underlying socket
// API calls failed.
serverSocketHandle_t CreateServerSocket(int port, int backlog);

// class ServerSocket
// Simple abstraction of a listening socket accepting incoming connections. Constructors are private
// and an instance can be obtained by calling the factroy function CreateServerSocket.
// EXCEPTIONS: If any error occurs durning a method call, the corresponding errno code is thrown (fixme :-)
class ServerSocket {
public:
	~ServerSocket();

	// Waits for incoming connections. Once a request is accepted, returns the handle to a connected
	// Socket
	socketHandle_t Accept();

	// Closes the ServerSocket
	void Close();
	
private:
	friend serverSocketHandle_t CreateServerSocket(int port, int backlog);
	ServerSocket(int port, int backlog);


	int port;
	int backlog;
	int sck;
	bool closed;
	int err;
};

} // rhs namespace

#endif
