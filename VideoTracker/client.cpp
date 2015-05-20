#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <sstream>

#include "Socket.hpp"
#include "msg.hpp"
#include "thread.hpp"

using namespace std;

extern void *Receiver(void *arg);

shared_ptr< vector< unique_ptr<Socket> > > connections;

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

	bool tracking = false;
	std::unique_ptr<thread> receiver(nullptr);

	for (;;) {
		string cmd, s;

		tokens.clear();
		do {
			getline(cin, s);
			split(s, ' ', tokens);
		} while (tokens.size() == 0);

		cmd = tokens[0];
		if (cmd == "connect") {
			if (tracking) {
				cout << "Cannot add a new connection while tracking\n";
			} else if (tokens.size() < 2) {
				cout << "Usage: connect server [port] (default port: " << rhs::SERVER_PORT_DEFAULT << ")\n";
			} else {
				string address = tokens[1];
				int port = tokens.size() > 2 ? stoi(tokens[2]) : rhs::SERVER_PORT_DEFAULT;
				cout << "Connecting to " << tokens[1] << ":" << (tokens.size() > 2 ? stoi(tokens[2]) : rhs::SERVER_PORT_DEFAULT) << "...\n";
				connections->push_back(unique_ptr<Socket>(new Socket(address, port)));
				cout << "Connected\n";
			}
		} else if (cmd == "start") {
			if (connections->size() == 0) {
				cout << "No active connections\n";
			} else if (tracking) {
				cout << "System already tracking\n";
			} else {
				receiver.reset(new thread(Receiver, 0));
				tracking = true;
			}
		} else if (cmd == "stop") {
			if (tracking) {
				for (auto &channel : *connections)
					channel->Send(rhs::Message(rhs::STOP_CAMERA));
				tracking = false;
				receiver->join();
			} else {
				cout << "Tracking not started yet!\n";
			}
		} else if (cmd == "close") {
			if (tracking) {
				for (auto &channel : *connections) {
					channel->Send(rhs::Message(rhs::STOP_CAMERA));
				}
				tracking = false;
				receiver->join();
			}
			break;
		} else {
			cout << "Unknown command '" << cmd << "'\n";
		}
	}

	for (auto &channel : *connections)
		channel->close();
}