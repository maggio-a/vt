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
#include <memory>
#include <string>
#include <cstdlib>
#include <unistd.h>

#include "calibration.hpp"
#include "ServerSocket.hpp"
#include "Socket.hpp"
#include "thread.hpp"
#include "Msg.hpp"
#include "Timer.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::unique_ptr;

using namespace rhs;

extern void *tracker2(void *arg);

// Boolean flag to control the camera thread
bool tracking = false;
// Communication channel with the remote controller
socketHandle_t channel;
// Run timer to obtain the timestamp of each capture
Timer live;
// Parameters of the camera :>
CameraParams params(rhs::CameraParamsPath);

void help(char *program) {
	cout << "Usage: " << program << " [-c width height]\n"
	     << " -c option to configure the coordinate transformation\n"
	     << "   width: width of the ground rectangle\n"
	     << "   height: height of the ground rectangle\n"
	     << "   the ground rectangle on the image plane must\n"
	     << "   be drawn according to the ground coordinates (0,0)->(width,0)->(width,height)->(0,height)\n";
}

int main(int argc, char *argv[]) {
	int opt;
	bool calibrate = false;
	while ((opt = getopt(argc, argv, "c")) != -1) {
		switch (opt) {
		case 'c':
			calibrate = true;
			break;
		default:
			help(argv[0]);
			std::exit(EXIT_FAILURE);
		}
	}

	if (calibrate) {
		help(argv[0]);
		if (optind > argc-2) {
			std::exit(EXIT_FAILURE);
		}
		float width = std::stof(argv[optind++]);
		float height = std::stof(argv[optind++]);
		rhs::PerformCalibration(width, height);
		std::exit(EXIT_SUCCESS);
	}

	// need to try-catch
	serverSocketHandle_t server = CreateServerSocket(rhs::SERVER_PORT_DEFAULT, 5);

	cout << "Camera server running" << endl;

	for (;;) { // handles 1 connection at a time
		bool done = false;
		channel = server->Accept();
		cout << "Connection accepted...\n";
	
		unique_ptr<rhs::thread> cam_service;
	
		while (!done) {
			try {
				Message msg = channel->Receive();
	
				switch (msg.type) {
				case Message::START_CAMERA:
					if (!tracking) {
						cout << "Starting capture\n";
						live.Restart();
						tracking = true;
						try {
							cam_service.reset(new rhs::thread(tracker2, 0));
						}
						catch (int error) {
							perror("rhs::thread");
						}
					}
					break;
				case Message::QUIT:
					done = true;
					//follows through
				case Message::STOP_CAMERA:
					if (tracking) {
						tracking = false;
						cam_service->join();
					}
					break;
				default:
					std::cerr << "Warning: message code unknown\n";
				}
			}
			catch (int status) {
				if (tracking) {
					tracking = false;
					cam_service->join();
				}
				if (status < 0) {
					errno = status;
					perror("Receive");
				}
				break;
			}
		} //while handling connection
	
		channel->Close();
		cout << "Disconnected\n";
	}

	server->Close();
	cout << "here\n";
	return 0;
}
