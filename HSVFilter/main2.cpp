#include <iostream>
#include <string>
#include <stdio.h>
#include <time.h>
#include <opencv2/opencv.hpp>

#ifdef __arm__

#include <raspicam/raspicam_cv.h>

#endif

using namespace std;
using namespace cv;

const int FRAME_WIDTH = 640;
const int FRAME_HEIGHT = 480;

const string windowName = "Feed";

Point last(0, 0);
vector<Point2i> ground;
bool tracing = false;

void on_mouse(int event, int x, int y, int, void*) {
    last = Point(x, y);
    if (tracing && event == EVENT_LBUTTONDOWN) {
        ground.push_back(Point2i(x, y));
        if (ground.size() == 4) {
            tracing = false;
            // and we can compute the homography
        }
    }
}

int main(int argc, char** argv )
{
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

    cv::Mat image;

    if (!cam.isOpened()) {
        cerr << "Failed to open the camera" << endl;
        return -1;
    }

    int keyCode;

    namedWindow(windowName);
    setMouseCallback(windowName, on_mouse, &image);

    int font = FONT_HERSHEY_PLAIN;
    double font_scale = 0.8;

    while (true) {
        cam.grab();
        cam.retrieve(image);
        circle(image, last, 5, Scalar(255,255,0), 1, CV_AA);
        
        if (ground.size() > 0) {
            for (size_t i = 0; i < ground.size(); ++i)
                circle(image, ground[i], 5, Scalar(255,255,0), 1, CV_AA);
            cv::polylines(image, ground, true, Scalar(0,255,0), 1, CV_AA);
        }
        int text_y = image.size().height - 10;
        cv::putText(image, "TRACING:", Point(5,text_y), font, font_scale, Scalar(255,255,255));
        if (tracing)
            cv::putText(image, "ON", Point(65,text_y), font, font_scale, Scalar(0,255,0));
        else
            cv::putText(image, "OFF", Point(65,text_y), font, font_scale, Scalar(0,0,255));
            
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

    return 0;
}
