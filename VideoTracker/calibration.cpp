#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>

#include <opencv2/opencv.hpp>
#ifdef __arm__
  #include <raspicam/raspicam_cv.h>
#endif

#include "calibration.hpp"

static const int FRAME_WIDTH = 640;
static const int FRAME_HEIGHT = 480;

static const string windowName = "Calibration";

static cv::Point2i last(0, 0);
static vector<cv::Point2i> ground;
static bool tracing = false;

static void onMouse(int event, int x, int y, int, void*) {
    last = Point(x, y);
    if (tracing && event == EVENT_LBUTTONDOWN) {
        ground.push_back(Point2i(x, y));
        if (ground.size() == 4) {
            tracing = false;
            // and we can compute the homography
        }
    }
}

void RHS::performCalibration() {
#ifdef __arm__
    raspicam::RaspiCam_Cv cam;
    cam.set(CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
    cam.set(CV_CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);
    cam.open();
#else
    cv::VideoCapture cam(0);
    cam.set(CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
    cam.set(CV_CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);
#endif
    if (!cam.isOpened()) {
        std::cerr << "Failed to open the camera" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    cv::namedWindow(windowName);
    cv::setMouseCallback(windowName, on_mouse, &image);

    cv::Mat image;
    int keyCode;
    int font = FONT_HERSHEY_PLAIN;
    double font_scale = 0.8;
    int text_y = FRAME_HEIGHT - 10;
    while (true) {
        cam.read(image);
        
        cv::circle(image, last, 5, cv::Scalar(255,255,0), 1, CV_AA);
        if (ground.size() > 0) {
            for (size_t i = 0; i < ground.size(); ++i)
                cv::circle(image, ground[i], 5, cv::Scalar(255,255,0), 1, CV_AA);
            cv::polylines(image, ground, true, cv::Scalar(0,255,0), 1, CV_AA);
        }
        cv::putText(image, "TRACING:", cv::Point2i(5,text_y), font, font_scale, cv::Scalar(255,255,255));
        if (tracing)
            cv::putText(image, "ON", cv::Point2i(65,text_y), font, font_scale, cv::Scalar(0,255,0));
        else
            cv::putText(image, "OFF", cv::Point2i(65,text_y), font, font_scale, cv::Scalar(0,0,255));   
        
        keyCode = cv::waitKey(1);
        if (keyCode == 32 || (keyCode & 0xff) == 32) {
            if (ground.size() == 4)
                ground.clear();
            tracing = !tracing; //toggle
        } else if (keyCode == 'r' || (keyCode & 0xff) == 'r') {
            if (ground.size() > 0)
                ground.pop_back();
        }
        cv::imshow(windowName, image);
    }

    cam.release();
    cv::destroyAllWindows();
}
