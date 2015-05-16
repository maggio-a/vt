#ifndef RHS_SERVERSOCKET_HDR
#define RHS_SERVERSOCKET_HDR

class Socket;

class ServerSocket {
public:
	ServerSocket(int port, int backlog);
	~ServerSocket();

	Socket * accept();
	void close();
	
private:
	int _port;
	int _backlog;
	int _sck;
	int _err;
	bool _closed;
};

#endif