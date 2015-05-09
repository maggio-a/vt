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

const std::string homographyPath = "homography.yaml";
void performCalibration(); 

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
		performCalibration();
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

#include <opencv2/opencv.hpp>

void performCalibration() {
	const int FRAME_WIDTH = 640;
	const int FRAME_Height = 480;
#ifdef __arm__
    raspicam::RaspiCam_Cv cam;
    cam.set(CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
    cam.set(CV_CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);
    cam.open();
#else
    cv::VideoCapture cam(0);
    //cam.set(CV_CAP_PROP_FPS, 80.0);
    cam.set(CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
    cam.set(CV_CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);
#endif

    const std::string windowName = "Calibration";

    cv::Mat image;

    if (!cam.isOpened()) {
        cerr << "Failed to open the camera" << endl;
        return -1;
    }

    int keyCode;

    namedWindow(windowName);
    setMouseCallback(windowName, on_mouse, 0);

    while (true) {
        cam.grab();
        cam.retrieve(image);
        cv::polylines(image, ground, true, Scalar(0,0,200), 1, CV_AA);
        cv::putText(image, "TRACING:", Point(5, 20), FONT_HERSHEY_SIMPLEX, 1.0, Scalar(255,255,255));
        if (tracing)
        	cv::putText(image, "ON", Point(35, 20), FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0,255,0));
        else
        	cv::putText(image, "OFF", Point(35, 20), FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0,0,255));
        	
        cv::imshow(windowName, image);
        keyCode = cv::waitKey(1);
        if (keyCode == 32 || (keyCode & 0xff) == 32) {
            if (ground.size() == 4)
                ground.clear();
            tracing = !tracing; //toggle
        }
    }

    cam.release();
    cv::destroyAllWindows();

    return 0;
}