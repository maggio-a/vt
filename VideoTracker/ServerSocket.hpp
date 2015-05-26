#ifndef RHS_SERVERSOCKET_HDR
#define RHS_SERVERSOCKET_HDR

#include <memory>
#include "Socket.hpp"

class ServerSocket;
typedef std::unique_ptr<ServerSocket> serverSocketHandle_t;

serverSocketHandle_t CreateServerSocket(int port, int backlog);

class ServerSocket {
public:
	~ServerSocket();

	socketHandle_t Accept();
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

#endif