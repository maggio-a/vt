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

void help(char *program) {
	std::cout << "Usage: " << program << " [-c width height]" << std::endl
	          << " -c option to configure the coordinate transformation" << std::endl
	          << "   width: width of the ground rectangle" << std::endl
	          << "   height: height of the ground rectangle" << std::endl
	          << "   the ground rectangle on the image plane must" << std::endl
	          << "   be drawn according to the ground coordinates (0,0)->(width,0)->(width,height)->(0,height)" << std::endl;
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
	channel.reset(server->accept());

	bool done = false;

	thread *cam_service;

	cout << "Camera server running" << endl;

	while (!done) {
		//msg_code cmd;
		rhs::Message msg(0); cout << "FIXME :(" << endl;

		try {
			cout << "receiving ";
			//channel->recv(&cmd, sizeof(cmd));
			msg = channel->Receive();
			cout << "cmd " << msg.type << endl;
		}
		catch (int status) {
			//cmd = QUIT; // forces quit
			msg.type = rhs::QUIT; // forces quit
			if (status < 0) {
				errno = status;
				perror("recv");
			} 
		}

		switch (msg.type) {
		case rhs::START_CAMERA:
			cout << msg.payload << endl;
			live.restart();
			if (!tracking) {
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
			std::cerr << "Warning: message code unknown" << std::endl;
		}
	}

	channel->close();
	server->close();
}
