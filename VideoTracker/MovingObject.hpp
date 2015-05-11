#ifndef RHS_MOVINGOBJECT_HDR
#define RHS_MOVINGOBJECT_HDR

#include <string>
#include <opencv2/opencv.hpp>

#include "Timer.hpp"

namespace rhs {

class MovingObject {
public:
    MovingObject(std::string tag, float x, float y);
    ~MovingObject();

    cv::Point2f predictPosition(float dt);
    void feedback(const cv::Mat &measurement);
    std::string tag() const;
private:
    friend struct outdated;

    cv::KalmanFilter kf;
    std::string _tag;
    Timer lastMeasurement;
};

MovingObject::MovingObject(std::string tag, float x, float y)
        : kf(4, 2, 0, CV_32F), _tag(tag), lastMeasurement() {

    /* transition matrix will be updated at each step:
        1   0  dt   0
        0   1   0  dt
        0   1   1   1
        0   0   0   1 */
    cv::setIdentity(kf.transitionMatrix);

    /* measurement is 
        1   0   0   0
        0   1   0   0 */
    kf.measurementMatrix = cv::Mat::zeros(2, 4, CV_32F);
    kf.measurementMatrix.at<float>(0,0) = 1.0f;
    kf.measurementMatrix.at<float>(1,1) = 1.0f;

    cv::setIdentity(kf.processNoiseCov, cv::Scalar(1e-2));
    cv::setIdentity(kf.measurementNoiseCov, cv::Scalar(1e-1));
    cv::setIdentity(kf.errorCovPost, cv::Scalar::all(1));
    kf.statePost = *(cv::Mat_<float>(4,1) << x, y, 0, 0);
}

MovingObject::~MovingObject() {

}

cv::Point2f MovingObject::predictPosition(float dt) {
    kf.transitionMatrix.at<float>(1,3) = dt;
    kf.transitionMatrix.at<float>(0,2) = dt;
    const cv::Mat &prediction = kf.predict();
    return cv::Point2f(prediction.at<float>(0), prediction.at<float>(1));
}

void MovingObject::feedback(const cv::Mat &measurement) {
    kf.correct(measurement);
    lastMeasurement.restart();
}

std::string MovingObject::tag() const {
    return _tag;
}

// predicate for purging undetected objects
struct outdated {
    float threshold;

    outdated(float th) : threshold(th) {

    }

    bool operator()(const MovingObject &obj) {
        return obj.lastMeasurement.timeElapsed() > threshold;
    }
};

} // rhs namespace

#endif