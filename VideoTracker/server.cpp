#include <iostream>
#include <memory>
#include <cstdlib>
#include <unistd.h>

#include "ServerSocket.hpp"
#include "Socket.hpp"
#include "thread.hpp"
#include "msg.hpp"

using std::cout;
using std::cerr;
using std::endl;

extern void *tracker(void *arg);
extern void *tracker2(void *arg);

bool tracking = false;

int main(int argc, char *argv[]) {

	int opt;
	boolean calibrate = false;
	while ((opt = getopt(argc, argv, "c")) != -1) {
		switch (opt) {
		case 'c':
			calibrate = true;
			break;
		default:
			help();
			exit(EXIT_FAILURE);
		}
	}

	if (calibrate) {
		rhs::performCalibration();
		cout << "Calibration completed" << endl;
		exit(EXIT_SUCCESS);
	}

	// need to try-catch
	std::auto_ptr<ServerSocket> server(new ServerSocket(12345, 5));
	std::auto_ptr<Socket> channel(server->accept());

	bool done = false;

	thread *cam_service;

	cout << "Camera server running" << endl;

	while (!done) {
		msg_code cmd;

		try {
			cout << "receiving";
			channel->recv(&cmd, sizeof(cmd));
			cout << "cmd" << endl;
		}
		catch (int status) {
			cmd = QUIT; // forces quit
			if (status < 0) {
				errno = status;
				perror("recv");
			} 
		}

		switch (cmd) {
		case START_CAMERA:
			if (!tracking) {
				tracking = true;
				try {
					cam_service = new thread(tracker, NULL);
				}
				catch (int error) {
					cerr << "disaster" << endl;
					errno = error;
					perror("thread");
				}
			}
			break;
		case QUIT:
			done = true;
		case STOP_CAMERA:
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
