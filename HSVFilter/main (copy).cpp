#include <iostream>
#include <string>
#include <stdio.h>
#include <time.h>
#include <opencv2/opencv.hpp>

#ifdef __arm__

#include <raspicam/raspicam_cv.h>

#endif

using namespace std;
using namerspace cv;

const int FRAME_WIDTH = 640;
const int FRAME_HEIGHT = 480;

const string windowName = "Feed";

vector<Point2i> ground;
bool tracing = false;
void on_mouse(int event, int x, int y, int, void*) {
    if (tracing && event == RBUTTON_DOWN) {
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

    createWindow(windowName);
    setMouseCallback(windowName, on_mouse, 0);

    while (true) {

        // Grab a frame
        clock_gettime(CLOCK_MONOTONIC, &t1);

        //cam >> image;
        cam.grab();
        cam.retrieve(image);
        cv::imwrite("source.jpg", image);

        clock_gettime(CLOCK_MONOTONIC, &t2);

        cv::cvtColor(image, HSV, CV_BGR2HSV);

        clock_gettime(CLOCK_MONOTONIC, &t3);

        cv::inRange(HSV, cv::Scalar(H_MIN,S_MIN,V_MIN), cv::Scalar(H_MAX,S_MAX,V_MAX), stencil);

        clock_gettime(CLOCK_MONOTONIC, &t4);

        //morphOps(stencil);
        cv::morphologyEx(stencil, stencil, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_RECT,cv::Size(5,5)));

        clock_gettime(CLOCK_MONOTONIC, &t5);

        // Find contours
        stencil.copyTo(tmp);
        //cv::findContours(tmp, cnt, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
        cv::findContours(tmp, cnt, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
        for (size_t i = 0; i < cnt.size(); i++) {
            cv::Rect b = cv::boundingRect(cnt[i]);
            cv::rectangle(image, b, cv::Scalar(0, 255, 0));
        }

        clock_gettime(CLOCK_MONOTONIC, &t_end);

        cout << getTimeDelta(t2, t1) << ", " <<
                getTimeDelta(t3, t2) << ", " <<
                getTimeDelta(t4, t3) << ", " <<
                getTimeDelta(t5, t4) << ", " <<
                getTimeDelta(t_end, t5) << ", " <<
                "Fps: " << 1.0 / getTimeDelta(t_end, t1) << endl;

        imshow(windowName, image);

        keyCode = cv::waitKey(1);
        if (keyCode == 32 || (keyCode & 0xff) == 32 ) {
            if (ground.size() == 4)
                ground.clear();
            tracing = !tracing; //toggle
        }
    }

    cam.release();
    cv::destroyAllWindows();

    return 0;
}
