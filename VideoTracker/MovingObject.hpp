#include <string>
#include <time.h>

#include <opencv2/opencv.hpp>
// FIXME duplicata (tracker e tracker2.cpp)
static inline float _getTimeDelta(const struct timespec &t2, const struct timespec &t1) {
    return ((t2.tv_sec*1000.0 + t2.tv_nsec/1000000.0) - (t1.tv_sec*1000.0 + t1.tv_nsec/1000000.0)) / 1000.0;
}

using cv::KalmanFilter;

class MovingObject {
public:
    MovingObject(std::string tag, float x, float y);
    ~MovingObject();

    cv::Point2i predictPosition(float dt);
    void feedback(const cv::Mat &measurement, const struct timespec &time);

private:
    friend struct outdated;

    cv::KalmanFilter _kf;
    std::string _tag;
    struct timespec _lastDetected;
};

MovingObject::MovingObject(std::string tag, float x, float y)
        : _kf(4, 2, 0, CV_32F), _tag(tag) {

    /* transition matrix will be updated at each step:
        1   0  dt   0
        0   1   0  dt
        0   1   1   1
        0   0   0   1 */
    cv::setIdentity(_kf.transitionMatrix);

    /* measurement is 
        1   0   0   0
        0   1   0   0 */
    _kf.measurementMatrix = cv::Mat::zeros(2, 4, CV_32F);
    _kf.measurementMatrix.at<float>(0,0) = 1.0f;
    _kf.measurementMatrix.at<float>(1,1) = 1.0f;

    cv::setIdentity(_kf.processNoiseCov, cv::Scalar(1e-2));
    cv::setIdentity(_kf.measurementNoiseCov, cv::Scalar(1e-1));
    cv::setIdentity(_kf.errorCovPost, cv::Scalar::all(1));
    _kf.statePost = *(cv::Mat_<float>(4,1) << x, y, 0, 0);

    clock_gettime(CLOCK_MONOTONIC, &_lastDetected);
}

MovingObject::~MovingObject() {

}

cv::Point2i MovingObject::predictPosition(float dt) {
    _kf.transitionMatrix.at<float>(1,3) = dt;
    _kf.transitionMatrix.at<float>(0,2) = dt;
    const cv::Mat &prediction = _kf.predict();
    return cv::Point2i(prediction.at<float>(0), prediction.at<float>(1));
}

void MovingObject::feedback(const cv::Mat &measurement, const struct timespec &mtime) {
    _kf.correct(measurement);
    _lastDetected = mtime;
}

// predicate for purging undetected objects
struct outdated {
    const struct timespec &time;
    float threshold;

    outdated(const struct timespec &t, float th) : time(t), threshold(th) { }
    bool operator()(const MovingObject &obj) {
        return _getTimeDelta(time, obj._lastDetected) > threshold;
    }
};