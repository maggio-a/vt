// =============================================================================
//
//  This file is part of the final project source code for the course "Ad hoc
//  and sensor networks" (Master's degree in Computer Science, University of
//  Pisa)
//
//  Copyright (C) 2015, Andrea Maggiordomo
//
// =============================================================================

#ifndef RHS_MOVINGOBJECT_HDR
#define RHS_MOVINGOBJECT_HDR

#include <string>
#include <cassert>
#include <opencv2/opencv.hpp>

#include "Timer.hpp"

namespace rhs {

// class MovingObject
// This class provides a wrapper for a kalman filter which models the state of a detected object
// in the scene.
// The state of an object is defined as its position and its speed, and the measurement matrix 
// is updated at each step with the time delta from the previous step.
class MovingObject {
public:
	// State of the underlying kalman filter in the predict-update cycle
	enum State { INITIALIZED, AFTER_PREDICT, AFTER_UPDATE };

	MovingObject(std::string tg, cv::Point2f pt) : state(INITIALIZED), kf(4, 2, 0, CV_32F), tag(tg), lastMeasurement() {
		/* transition matrix will be updated at each step:
			1   0  dt   0
			0   1   0  dt
			0   0   1   0
			0   0   0   1 */
		cv::setIdentity(kf.transitionMatrix);
	
		/* measurement is 
			1   0   0   0
			0   1   0   0
		   (speed is not measured) */
		kf.measurementMatrix = cv::Mat::zeros(2, 4, CV_32F);
		kf.measurementMatrix.at<float>(0,0) = 1.0f;
		kf.measurementMatrix.at<float>(1,1) = 1.0f;
	
		cv::setIdentity(kf.processNoiseCov, cv::Scalar(1e-2));
		cv::setIdentity(kf.measurementNoiseCov, cv::Scalar(1e-1));
		cv::setIdentity(kf.errorCovPost, cv::Scalar::all(1));
		kf.statePost = *(cv::Mat_<float>(4,1) << pt.x, pt.y, 0, 0);
	}


	~MovingObject() {  }

	// Predicts the position of the object dt seconds after the last step
	// the parameter dt can - of course - be a fractional value
	cv::Point2f PredictPosition(float dt) {
		kf.transitionMatrix.at<float>(1,3) = dt;
		kf.transitionMatrix.at<float>(0,2) = dt;
		const cv::Mat &prediction = kf.predict();
		cv::Mat diff = prediction - kf.statePre;
		assert (cv::countNonZero(diff) == 0);
		state = AFTER_PREDICT;
		return cv::Point2f(prediction.at<float>(0), prediction.at<float>(1));
	}

	// Corrects the Kalman filter with the given measurement
	void Feedback(const cv::Mat &measurement) {
		kf.correct(measurement);
		lastMeasurement.Restart();
		state = AFTER_UPDATE;
	}

	// Returns the coordinates estimated at the last step
	cv::Point2f GetEstimatePre() const {
		return cv::Point2f(kf.statePre.at<float>(0), kf.statePre.at<float>(1));
	}

	// Returns the coordinates estimated after the state has been corrected at the current step
	// This is meaningless if MovingObject::Feedback was not called after MovingObject::PredictPosition
	// (maybe because no measurement was available)
	cv::Point2f GetEstimatePost() const {
		return cv::Point2f(kf.statePost.at<float>(0), kf.statePost.at<float>(1));
	}

	// Returns the state of the Kalman filter in the predict-update cycle
	State GetKfState() const { return state; }

	// Returns the tag given to this object
	std::string Tag() const { return tag; }

	// Time elapsed since the last measurement has been applied (FIXME: THIS DOES NOT BELONG HERE)
	float TimeFromLastMeasurement() const { return lastMeasurement.TimeElapsed(); }

private:
	State state;
	cv::KalmanFilter kf;
	std::string tag;
	Timer lastMeasurement;
};

} // rhs namespace

#endif