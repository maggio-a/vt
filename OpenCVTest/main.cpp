#include <stdio.h>
#include <time.h>
#include <opencv2/opencv.hpp>

#ifdef __arm__

#include <raspicam/raspicam_cv.h>

#endif

#define BACKGROUNDSUB_COLOR 0

#define asd

using namespace std;

/* Assumes t2 > t1 */
double getTimeDelta(const struct timespec &t2, const struct timespec &t1)
{
    return ((t2.tv_sec*1000.0 + t2.tv_nsec/1000000.0) - (t1.tv_sec*1000.0 + t1.tv_nsec/1000000.0)) / 1000.0;
}

int main(int argc, char** argv )
{
#ifdef __arm__
    raspicam::RaspiCam_Cv cam;
    cam.set(CV_CAP_PROP_FRAME_WIDTH, 640.0);
    cam.set(CV_CAP_PROP_FRAME_HEIGHT, 480.0);
    cam.open();
#else
    cv::VideoCapture cam(0);
    //cam.set(CV_CAP_PROP_FPS, 80.0);
    cam.set(CV_CAP_PROP_FRAME_WIDTH, 1280.0);
    cam.set(CV_CAP_PROP_FRAME_HEIGHT, 720.0);
#endif

    cv::Mat image, gray, fgmask, tmp;

    if (!cam.isOpened()) {
        cerr << "Failed to open the camera" << endl;
        return -1;
    }

    cv::BackgroundSubtractorMOG bgs(10, 3, 0.6, 20);

    cv::Mat probe = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));

    std::vector< std::vector<cv::Point> > cnt;
    //std::vector<cv::Vec4i> hierarchy;

    int keyCode;

    struct timespec t1, t2, t3, t4, t5, t6;
    struct timespec t_end;

    while (true) {
        cam.grab();
        cam.retrieve(image);
        cv::imshow("Camera feed", image);
        keyCode = cv::waitKey(1) & 0xFF;
        if (keyCode == 32) // Space
            break;
    }

    while (true) {

        // Grab a frame
        clock_gettime(CLOCK_MONOTONIC, &t1);

        //cam >> image;
        cam.grab();
        cam.retrieve(image);

        clock_gettime(CLOCK_MONOTONIC, &t2);

        cv::cvtColor(image, gray, CV_BGR2GRAY);

        clock_gettime(CLOCK_MONOTONIC, &t3);

        cv::medianBlur(gray, gray, 5);

        clock_gettime(CLOCK_MONOTONIC, &t4);
#if BACKGROUNDSUB_COLOR == 1
        bgs(image, fgmask);
#else
        bgs(gray, fgmask);
#endif

        clock_gettime(CLOCK_MONOTONIC, &t5);

        cv::morphologyEx(fgmask, fgmask, cv::MORPH_OPEN, probe);

        clock_gettime(CLOCK_MONOTONIC, &t6);

        // Find contours
        fgmask.copyTo(tmp);
        //cv::findContours(tmp, cnt, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
        cv::findContours(tmp, cnt, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
        for (size_t i = 0; i < cnt.size(); i++) {
            cv::Rect b = cv::boundingRect(cnt[i]);
            cv::rectangle(image, b, cv::Scalar(0, 255, 0));
        }

        clock_gettime(CLOCK_MONOTONIC, &t_end);

        cout << getTimeDelta(t2, t1) << " " <<
                getTimeDelta(t3, t2) << " " <<
                getTimeDelta(t4, t3) << " " <<
                getTimeDelta(t5, t4) << " " <<
                getTimeDelta(t6, t5) << " " <<
                getTimeDelta(t_end, t6) << " " <<
                "Fps: " << 1.0 / getTimeDelta(t_end, t1) << endl;

        cv::imshow("Foreground", fgmask);
        cv::imshow("Camera feed", image);
        cv::imshow("Grayscale", gray);

        keyCode = cv::waitKey(1);
        if (keyCode == 32 || (keyCode & 0xff) == 32 )
            break;
    }

    cam.release();
    cv::destroyAllWindows();

    return 0;
}
