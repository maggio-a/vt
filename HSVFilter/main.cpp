#include <iostream>
#include <string>
#include <stdio.h>
#include <time.h>
#include <opencv2/opencv.hpp>

#ifdef __arm__

#include <raspicam/raspicam_cv.h>

#endif

using namespace std;

int H_MIN = 0;
int H_MAX = 256;
int S_MIN = 0;
int S_MAX = 256;
int V_MIN = 0;
int V_MAX = 256;

const int FRAME_WIDTH = 640;
const int FRAME_HEIGHT = 480;

const int MAX_NUM_OBJECTS = 50;

//const int MIN_OBJECT_AREA = 20*20;
//const int MAX_OBJECT_AREA = FRAME_HEIGHT*FRAME_WIDTH/1.5;

const string windowName = "Original Image";
const string windowName1 = "HSV Image";
const string windowName2 = "Thresholded Image";
const string windowName3 = "After Morphological Operations";
const string trackbarWindowName = "Trackbars";

void on_trackbar(int, void*) { }

void createTrackbars()
{
    cv::namedWindow(trackbarWindowName, 0);   
    cv::createTrackbar("H_MIN", trackbarWindowName, &H_MIN, H_MAX, on_trackbar);
    cv::createTrackbar("H_MAX", trackbarWindowName, &H_MAX, H_MAX, on_trackbar);
    cv::createTrackbar("S_MIN", trackbarWindowName, &S_MIN, S_MAX, on_trackbar);
    cv::createTrackbar("S_MAX", trackbarWindowName, &S_MAX, S_MAX, on_trackbar);
    cv::createTrackbar("V_MIN", trackbarWindowName, &V_MIN, V_MAX, on_trackbar);
    cv::createTrackbar("V_MAX", trackbarWindowName, &V_MAX, V_MAX, on_trackbar);
}

void morphOps(cv::Mat &thresh){
    //create structuring element that will be used to "dilate" and "erode" image.
    //the element chosen here is a 3px by 3px rectangle
    cv::Mat erodeElement = getStructuringElement(cv::MORPH_RECT, cv::Size(3,3));
    //dilate with larger element so make sure object is nicely visible
    cv::Mat dilateElement = getStructuringElement(cv::MORPH_RECT, cv::Size(8,8));

    cv::erode(thresh,thresh,erodeElement);
    cv::erode(thresh,thresh,erodeElement);
    cv::dilate(thresh,thresh,dilateElement);
    cv::dilate(thresh,thresh,dilateElement);
}

/* Assumes t2 > t1 */
double getTimeDelta(const struct timespec &t2, const struct timespec &t1)
{
    return ((t2.tv_sec*1000.0 + t2.tv_nsec/1000000.0) - (t1.tv_sec*1000.0 + t1.tv_nsec/1000000.0)) / 1000.0;
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

    cv::Mat image, HSV, stencil, tmp;

    if (!cam.isOpened()) {
        cerr << "Failed to open the camera" << endl;
        return -1;
    }

    createTrackbars();

    std::vector< std::vector<cv::Point> > cnt;
    //std::vector<cv::Vec4i> hierarchy;

    int keyCode;

    struct timespec t1, t2, t3, t4, t5, t6;
    struct timespec t_end;

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
        imshow(windowName1, HSV);
        imshow(windowName2, stencil);

        keyCode = cv::waitKey(1);
        if (keyCode == 32 || (keyCode & 0xff) == 32 ) {
            cv::imwrite("hsv.jpg", HSV);
            cv::imwrite("stencil.jpg", stencil);
            cv::imwrite("tracked.jpg", image);
            break;
        }
    }

    cam.release();
    cv::destroyAllWindows();

    return 0;
}
