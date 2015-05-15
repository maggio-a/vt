#include <iostream>
#include <memory>
#include <string>

#include "Socket.hpp"
#include "msg.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::cin;
using std::string;

int main(int argc, char *argv[]) {
	// need to try-catch
	string address = "127.0.0.1";
	std::auto_ptr<Socket> channel(new Socket(address, 12345));

	string s;
	//msg_code cmd = STOP_CAMERA;

	for (;;) {
		cin >> s;
		if (s == "start") {
			channel->Send(rhs::Message(rhs::START_CAMERA, string("Hello!")));
		} else if (s == "stop" || s == "close") {
			channel->Send(rhs::Message(rhs::STOP_CAMERA));
		} else if (s == "quit") {
			channel->Send(rhs::Message(rhs::QUIT));
		} else {
			cerr << "Unknown command '" << s << "'" << endl;
		}

		//channel->send(&cmd, sizeof(cmd));
		if (s == "close")
			break;
	}

	channel->close();
}