#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <opencv2/opencv.hpp>
#ifdef __arm__
  #include <raspicam/raspicam_cv.h>
#endif

#include "calibration.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::vector;

using namespace cv;

static const int FRAME_WIDTH = 640;
static const int FRAME_HEIGHT = 480;

static const string windowName = "Calibration";

static Point2i last(0, 0);
static vector<Point2i> img_quad; // ground rectangle in image coordinates
static bool tracing = false;

static void onMouse(int event, int x, int y, int, void*) {
    last.x = x, last.y = y;
    if (tracing && event == EVENT_LBUTTONDOWN) {
        img_quad.push_back(Point2i(x,y));
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
    VideoCapture cam(0);
    cam.set(CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
    cam.set(CV_CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);
#endif
    if (!cam.isOpened()) {
        cerr << "Failed to open the camera" << endl;
        exit(EXIT_FAILURE);
    }

    namedWindow(windowName);
    setMouseCallback(windowName, onMouse, 0);

    Mat image;
    int keyCode;
    int font = FONT_HERSHEY_PLAIN;
    double font_scale = 0.8;
    int text_y = FRAME_HEIGHT - 10;
    while (true) {
        cam.read(image);
        
        circle(image, last, 5, Scalar(255,255,0), 1, CV_AA);
        if (img_quad.size() > 0) {
            for (size_t i = 0; i < img_quad.size(); ++i)
                circle(image, img_quad[i], 5, Scalar(255,255,0), 1, CV_AA);
            polylines(image, img_quad, true, Scalar(0,255,0), 1, CV_AA);
        }
        putText(image, "TRACING:", Point2i(5,text_y), font, font_scale, Scalar(255,255,255));
        if (tracing)
            putText(image, "ON", Point2i(65,text_y), font, font_scale, Scalar(0,255,0));
        else
            putText(image, "OFF", Point2i(65,text_y), font, font_scale, Scalar(0,0,255));   
        
        keyCode = waitKey(1);
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
        imshow(windowName, image);
    }

    // Warning! insertion order determines the compuyted homography
    vector<Point2f> world_quad; // ground rectangle in world coordinates
    world_quad.push_back(Point2f(0.0f,0.0f));
    world_quad.push_back(Point2f(width,0.0f));
    world_quad.push_back(Point2f(width,height));
    world_quad.push_back(Point2f(0.0f,height));

    if (img_quad.size() != 4) {
        cerr << "Calibration error: 4 control points required" << endl;
        std::exit(EXIT_FAILURE);
    } else {
        // cv::getPerspectiveTransform wants an array of float coordinates
        vector<Point2f> img_quad_float;
        for (size_t i = 0; i < img_quad.size(); ++i) {
            img_quad_float.push_back(Point2i(img_quad[i]));
        }
        Mat img2world = getPerspectiveTransform(img_quad_float, world_quad);
        FileStorage fs(rhs::PathToCalibrationData, FileStorage::WRITE);
        if (fs.isOpened()) {
            fs << rhs::PerspectiveTransformationName << img2world;
            fs.release();
            cout << "Calibration data written to " << rhs::PathToCalibrationData << endl;
        } else {
            cerr << "Unable to write calibration data to " << rhs::PathToCalibrationData << endl;
            std::exit(EXIT_FAILURE);
        }
    }

    cam.release();
    destroyAllWindows();
}
