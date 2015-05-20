#include <iostream>
#include <memory>
#include <string>
#include <cstdlib>
#include <unistd.h>

#include "calibration.hpp"
#include "ServerSocket.hpp"
#include "Socket.hpp"
#include "thread.hpp"
#include "msg.hpp"
#include "Timer.hpp"

using std::cout;
using std::cerr;
using std::endl;

extern void *tracker(void *arg);
extern void *tracker2(void *arg);

bool tracking = false;
std::auto_ptr<Socket> channel(0);
rhs::Timer live;
rhs::CameraParams params(rhs::CameraParamsPath);

void help(char *program) {
	std::cout << "Usage: " << program << " [-c width height]\n"
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
		rhs::performCalibration(width, height);
		std::exit(EXIT_SUCCESS);
	}

	// need to try-catch
	std::auto_ptr<ServerSocket> server(new ServerSocket(12345, 5));

	cout << "Camera server running" << endl;

	while (true) { // handles 1 connection at a time
		bool done = false;
		channel.reset(server->accept());
		cout << "Connection accepted...\n";
	
		thread *cam_service;
	
		while (!done) {
			try {
				rhs::Message msg = channel->Receive();
	
				switch (msg.type) {
				case rhs::START_CAMERA:
					if (!tracking) {
						cout << "Starting capture\n";
						live.restart();
						tracking = true;
						try {
							cam_service = new thread(tracker2, 0);
						}
						catch (int error) {
							cerr << "disaster" << endl;
							errno = error;
							perror("thread");
						}
					}
					break;
				case rhs::QUIT:
					done = true;
				case rhs::STOP_CAMERA:
					if (tracking) {
						tracking = false;
						cam_service->join();
						delete cam_service;
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
					delete cam_service;
				}
				if (status < 0) {
					errno = status;
					perror("Receive");
				}
				break;
			}
		} //while handling connection
	
		channel->close();
		cout << "Disconnected\n";
	}

	server->close();
}
