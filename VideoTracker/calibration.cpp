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

static const std::string windowName = "Calibration";

static cv::Point2i last(0, 0);
static std::vector<cv::Point2i> img_quad; // ground rectangle in image coordinates
static bool tracing = false;

static void onMouse(int event, int x, int y, int, void*) {
    last.x = x, last.y = y;
    if (tracing && event == cv::EVENT_LBUTTONDOWN) {
        img_quad.push_back(cv::Point2i(x,y));
        if (img_quad.size() == 4) {
            tracing = false;
        }
    }
}

void rhs::performCalibration(float width, float height) {
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
    cv::setMouseCallback(windowName, onMouse, 0);

    cv::Mat image;
    int keyCode;
    int font = cv::FONT_HERSHEY_PLAIN;
    double font_scale = 0.8;
    int text_y = FRAME_HEIGHT - 10;
    while (true) {
        cam.read(image);
        
        cv::circle(image, last, 5, cv::Scalar(255,255,0), 1, CV_AA);
        if (img_quad.size() > 0) {
            for (size_t i = 0; i < img_quad.size(); ++i)
                cv::circle(image, img_quad[i], 5, cv::Scalar(255,255,0), 1, CV_AA);
            cv::polylines(image, img_quad, true, cv::Scalar(0,255,0), 1, CV_AA);
        }
        cv::putText(image, "TRACING:", cv::Point2i(5,text_y), font, font_scale, cv::Scalar(255,255,255));
        if (tracing)
            cv::putText(image, "ON", cv::Point2i(65,text_y), font, font_scale, cv::Scalar(0,255,0));
        else
            cv::putText(image, "OFF", cv::Point2i(65,text_y), font, font_scale, cv::Scalar(0,0,255));   
        
        keyCode = cv::waitKey(1);
        if (keyCode == ' ' || (keyCode & 0xff) == ' ') {
            if (img_quad.size() == 4)
                img_quad.clear();
            tracing = !tracing; //toggle
        } else if (keyCode == 'r' || (keyCode & 0xff) == 'r') {
            if (img_quad.size() > 0)
                img_quad.pop_back();
        } else if ((keyCode == '\n' || (keyCode & 0xff) == '\n')) { // Enter key
            break;
        }
        cv::imshow(windowName, image);
    }

    // Warning! insertion order determines the compuyted homography
    std::vector<cv::Point2f> world_quad; // ground rectangle in world coordinates
    world_quad.push_back(cv::Point2f(0.0f,0.0f));
    world_quad.push_back(cv::Point2f(width,0.0f));
    world_quad.push_back(cv::Point2f(width,height));
    world_quad.push_back(cv::Point2f(0.0f,height));

    if (img_quad.size() != 4) {
        std::cerr << "Calibration error: 4 control points required" << std::endl;
        std::exit(EXIT_FAILURE);
    } else {
        // cv::getPerspectiveTransform wants an array of float coordinates
        std::vector<cv::Point2f> img_quad_float;
        for (size_t i = 0; i < img_quad.size(); ++i) {
            img_quad_float.push_back(cv::Point2i(img_quad[i]));
        }
        cv::Mat img2world = cv::getPerspectiveTransform(img_quad_float, world_quad);
        cv::FileStorage fs(rhs::PathToCalibrationData, cv::FileStorage::WRITE);
        if (fs.isOpened()) {
            fs << rhs::PerspectiveTransformationName << img2world;
            fs.release();
            std::cout << "Calibration data written to" << rhs::PathToCalibrationData << std::endl;
        } else {
            std::cerr << "Unable to write calibration data to " << rhs::PathToCalibrationData << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    cam.release();
    cv::destroyAllWindows();
}
