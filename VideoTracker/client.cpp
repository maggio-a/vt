// =============================================================================
//
//  This file is part of the final project source code for the course "Ad hoc
//  and sensor networks" (Master's degree in Computer Science, University of
//  Pisa)
//
//  Copyright (C) 2015, Andrea Maggiordomo
//
// =============================================================================

#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <sstream>

#include "Socket.hpp"
#include "Msg.hpp"
#include "thread.hpp"

using namespace std;
using namespace rhs;

extern void *Receiver(void *arg);

shared_ptr< vector<socketHandle_t> > connections;

// width and height of the region of interest
// an object position p is displayed if 0 <= p.x <= ROI[0] and 0 <= p.y <= ROI[1]
float ROI[] = { 0.0f, 0.0f }; 

// Resolution of the display window
int res[] = { 300, 300 };

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
	if (argc < 3) {
		cerr << "Usage: " << argv[0] << " ROI_X ROI_Y [windowWidth] [windowHeight]\n";
		return -1;
	} else {
		float fv;
		ROI[0] = ((fv = stof(argv[1])) > 0.0f) ? fv : 0.0f;
		if (fv <= 0.0f)
			cout << "Warning: ROI_X discarded (ROI_X <= 0)\n";

		ROI[1] = ((fv = stof(argv[2])) > 0.0f) ? fv : 0.0f;
		if (fv <= 0.0f)
			cout << "Warning: ROI_Y discarded (ROI_X <= 0)\n";

		int iv;
		if (argc >= 4 && (iv = stoi(argv[3])) > 0)
			res[0] = iv;
		if (argc >= 5 && (iv = stoi(argv[4])) > 0)
			res[1] = iv;
	}

	connections.reset(new vector<socketHandle_t>);
	vector<string> tokens;

	bool tracking = false;
	std::unique_ptr<thread> receiver;

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
				connections->push_back(CreateSocket(address, port));
				cout << "Connected\n";
			}
		} else if (cmd == "start") {
			if (connections->size() == 0) {
				cout << "No active connections\n";
			} else if (tracking) {
				cout << "System already tracking\n";
			} else {
				// activate remote sensors
				for (auto &channel : *connections) {
					channel->Send(Message(Message::START_CAMERA));
				}
				receiver.reset(new thread(Receiver, 0));
				tracking = true;
			}
		} else if (cmd == "stop") {
			if (tracking) {
				for (auto &channel : *connections)
					channel->Send(Message(Message::STOP_CAMERA));
				tracking = false;
				receiver->join();
			} else {
				cout << "Tracking not started yet!\n";
			}
		} else if (cmd == "close") {
			if (tracking) {
				for (auto &channel : *connections) {
					channel->Send(Message(Message::QUIT));
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
		channel->Close();

	return 0;
}
