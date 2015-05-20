#ifndef RHS_MOVINGOBJECT_HDR
#define RHS_MOVINGOBJECT_HDR

#include <string>
#include <opencv2/opencv.hpp>

#include "Timer.hpp"

namespace rhs {

class MovingObject {
public:
	enum State { INITIALIZED, AFTER_PREDICT, AFTER_UPDATE };

	MovingObject(std::string tag, cv::Point2f pt) : state(INITIALIZED), kf(4, 2, 0, CV_32F), objTag(tag), lastMeasurement() {
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
		kf.statePost = *(cv::Mat_<float>(4,1) << pt.x, pt.y, 0, 0);
	}


	~MovingObject() {  }

	cv::Point2f predictPosition(float dt) {
		kf.transitionMatrix.at<float>(1,3) = dt;
		kf.transitionMatrix.at<float>(0,2) = dt;
		const cv::Mat &prediction = kf.predict();
		cv::Mat diff = prediction - kf.statePre;
		assert (cv::countNonZero(diff) == 0);
		state = AFTER_PREDICT;
		return cv::Point2f(prediction.at<float>(0), prediction.at<float>(1));
	}

	cv::Point2f getEstimatePre() const {
		return cv::Point2f(kf.statePre.at<float>(0), kf.statePre.at<float>(1));
	}

	cv::Point2f getEstimatePost() const {
		return cv::Point2f(kf.statePost.at<float>(0), kf.statePost.at<float>(1));
	}

	void feedback(const cv::Mat &measurement) {
		kf.correct(measurement);
		lastMeasurement.restart();
		state = AFTER_UPDATE;
	}

	State getObjectState() const { return state; }

	std::string tag() const { return objTag; }

	float timeFromLastMeasurement() const { return lastMeasurement.timeElapsed(); }


private:
	State state;
	cv::KalmanFilter kf;
	std::string objTag;
	Timer lastMeasurement;
};

} // rhs namespace

#endif