#include <iostream>
#include <string>
#include <stdio.h>
#include <time.h>
#include <opencv2/opencv.hpp>

#ifdef __arm__

#include <raspicam/raspicam_cv.h>

#endif

extern bool tracking;

using std::cout;
using std::cerr;
using std::endl;
using std::string;


static int H_MIN = 0;
static int H_MAX = 256;
static int S_MIN = 0;
static int S_MAX = 256;
static int V_MIN = 0;
static int V_MAX = 256;

static const int FRAME_WIDTH = 640;
static const int FRAME_HEIGHT = 480;

static const int MAX_NUM_OBJECTS = 50;

//const int MIN_OBJECT_AREA = 20*20;
//const int MAX_OBJECT_AREA = FRAME_HEIGHT*FRAME_WIDTH/1.5;

static const string windowName = "Original Image";
static const string windowName1 = "HSV Image";
static const string windowName2 = "Thresholded Image";
static const string windowName3 = "After Morphological Operations";
static const string trackbarWindowName = "Trackbars";

static void on_trackbar(int, void*) { }

static void createTrackbars() {
    cv::namedWindow(trackbarWindowName, 0);   
    cv::createTrackbar("H_MIN", trackbarWindowName, &H_MIN, H_MAX, on_trackbar);
    cv::createTrackbar("H_MAX", trackbarWindowName, &H_MAX, H_MAX, on_trackbar);
    cv::createTrackbar("S_MIN", trackbarWindowName, &S_MIN, S_MAX, on_trackbar);
    cv::createTrackbar("S_MAX", trackbarWindowName, &S_MAX, S_MAX, on_trackbar);
    cv::createTrackbar("V_MIN", trackbarWindowName, &V_MIN, V_MAX, on_trackbar);
    cv::createTrackbar("V_MAX", trackbarWindowName, &V_MAX, V_MAX, on_trackbar);
}

/* Assumes t2 > t1 */
static double getTimeDelta(const struct timespec &t2, const struct timespec &t1) {
    return ((t2.tv_sec*1000.0 + t2.tv_nsec/1000000.0) - (t1.tv_sec*1000.0 + t1.tv_nsec/1000000.0)) / 1000.0;
}

void *tracker(void *arg) {

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
        return 0;
    }

    createTrackbars();

    std::vector< std::vector<cv::Point> > cnt;
    //std::vector<cv::Vec4i> hierarchy;

    int key_code;

    struct timespec t_prev, t_curr;

    /* kalman filter section */
    /* state will be [ pos_x, pos_y, speed_x, speed_y ]
       measurement will be [ pos_x, pos_y ]
       no control */
    cv::KalmanFilter kalman(4, 2, 0, CV_32F);

    /* transition matrix will be updated at each loop:
       1   0  dt   0
       0   1   0  dt
       0   1   1   1
       0   0   0   1 */
    cv::setIdentity(kalman.transitionMatrix);

    /* measurement is 
       1   0   0   0
       0   1   0   0 */
    kalman.measurementMatrix = cv::Mat::zeros(2, 4, CV_32F);
    kalman.measurementMatrix.at<float>(0,0) = 1.0f;
    kalman.measurementMatrix.at<float>(1,1) = 1.0f;

    /* FIXME */
    cv::setIdentity(kalman.processNoiseCov, cv::Scalar(0.001f));
    cv::setIdentity(kf.measurementNoiseCov, cv::Scalar(0.1f));
    cv::setIdentity(kf.errorCovPost, Scalar::all(1));
    kalman.statePost = *(Mat_<float>(4,1) << 20.0f, 20.0f, 0, 0);

    Mat measurement(2, 1, CV_32F);

    clock_gettime(CLOCK_MONOTONIC, &t_prev);

    while (tracking) {
        // Grab a frame
        //cam >> image;
        cam.grab();
        clock_getTime(CLOCK_MONOTONIC, &t_curr);
        cam.retrieve(image);

        float dt = float(getTimeDelta(t_curr, t_prev));

        cv::cvtColor(image, HSV, CV_BGR2HSV);
        cv::inRange(HSV, cv::Scalar(H_MIN,S_MIN,V_MIN), cv::Scalar(H_MAX,S_MAX,V_MAX), stencil);
        cv::morphologyEx(stencil, stencil, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_RECT,cv::Size(5,5)));

        // Find contours
        stencil.copyTo(tmp);
        //cv::findContours(tmp, cnt, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
        cv::findContours(tmp, cnt, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
        std::vector<cv::Point> *maxCnt = 0;
        for (size_t i = 0; i < cnt.size(); i++) {
            cv::Rect b = cv::boundingRect(cnt[i]);
            cv::rectangle(image, b, cv::Scalar(0, 255, 0));
            if (!maxCnt || (cv::boundingRect(*maxCnt)).area() < b.area()) {
                maxCnt = &cnt[i];
            }
        }

        kalman.measurementMatrix.at<float>(0,2) = dt;
        kalman.measurementMatrix.at<float>(1,3) = dt;

        if (maxCnt) {
            cv::Rect b = cv::boundingRect(*maxCnt);
            measurement.at<float>(0) = b.x + b.width / 2.0f;
            measurement.at<float>(1) = b.y + b.height / 2.0f;
            
        }

        imshow(windowName, image);
        imshow(windowName1, HSV);
        imshow(windowName2, stencil);

        key_code = cv::waitKey(1);
        if (key_code == 32 || (key_code & 0xff) == 32 ) {
            // skip
        }

        t_prev = t_curr;
    }

    cam.release();
    cv::destroyAllWindows();

    return 0;
}
