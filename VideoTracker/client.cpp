#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <sstream>

#include "Socket.hpp"
#include "msg.hpp"
#include "thread.hpp"

using namespace std;

std::auto_ptr<thread> receiver(0);

shared_ptr< vector< unique_ptr<Socket> > > connections;

extern void *Receiver(void *arg);

static void split(const string &s, char delimiter, vector<string> &tokens_out) {
	tokens_out.clear();
	stringstream ss(s);
	string tk;
	while (getline(ss, tk, delimiter)) {
		tokens_out.push_back(tk);
	}
}

int main(int argc, char *argv[]) {
	// need to try-catch
	//string address = "127.0.0.1";
	//string address = "192.168.1.72";
	//channel.reset(new Socket(address, 12345));

	connections.reset(new vector< unique_ptr<Socket> >);
	vector<string> tokens;
	//msg_code cmd = STOP_CAMERA;

	for (;;) {
		string cmd, s;
		getline(cin, s);
		split(s, ' ', tokens);
		cmd = tokens[0];
		if (cmd == "connect") {
			if (tokens.size() < 2) {
				cerr << "Usage: connect server [port] (default port: " << rhs::SERVER_PORT_DEFAULT << ")\n";
			} else {
				string address = tokens[1];
				int port = tokens.size() > 2 ? stoi(tokens[2]) : rhs::SERVER_PORT_DEFAULT;
				connections->push_back(unique_ptr<Socket>(new Socket(address, port)));
			}
		} else if (cmd == "start") {
			receiver.reset(new thread(Receiver, 0));
		} else if (cmd == "stop" || cmd == "close") {
			channel->Send(rhs::Message(rhs::STOP_CAMERA));
		} else if (cmd == "quit") {
			channel->Send(rhs::Message(rhs::QUIT));
		} else {
			cerr << "Unknown command '" << cmd << "'" << endl;
		}

		//channel->send(&cmd, sizeof(cmd));
		if (cmd == "close")
			break;
	}

	channel->close();
}