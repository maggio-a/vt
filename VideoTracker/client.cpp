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
	msg_code cmd = STOP_CAMERA;

	for (;;) {
		cin >> s;
		if (s == "start") {
			cmd = START_CAMERA;
		} else if (s == "stop" || s == "close") {
			cmd = STOP_CAMERA;
		} else if (s == "quit") {
			cmd = QUIT;
		} else {
			cerr << "Unknown command '" << s << "'" << endl;
		}

		channel->send(&cmd, sizeof(cmd));
		if (s == "close")
			break;
	}

	channel->close();
}