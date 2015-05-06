#include <string>

#include <opencv2/opencv.hpp>

using cv::kalman;

class MovingObject {
public:
    MovingObject(string tag, float x, float y);
    ~MovingObject();

    cv::Point2i predictPosition();
    void feedback(const cv::Mat &measurement);

private:
    cv::KalmanFilter _kf;
    string _tag;
};

MovingObject::MovingObject(string tag)
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
}

MovingObject::~MovingObject() {

}

cv::Point2i predictPosition(float dt) {
    _kf.transitionMatrix.at<float>(1,3) = dt;
    _kf.transitionMatrix.at<float>(0,2) = dt;
    cv::Mat &prediction = _kf.predict();
    return cv::Point2i(prediction.at<float>(0), prediction.at<float>(1));
}

void feedback(const cv::Mat &measurement) {
    _kf.update(measurement);
}
