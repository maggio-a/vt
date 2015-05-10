#include <iostream>
#include <string>
#include <cstdio>
#include <opencv2/opencv.hpp>
#ifdef __arm__
  #include <raspicam/raspicam_cv.h>
#endif

#include "ColorBasedDetector.hpp"
#include "MovingObject.hpp"
#include "Timer.hpp"
#include "calibration.hpp"

extern bool tracking;

using std::cout;
using std::cerr;
using std::endl;
using std::string;


static int H_MIN = 0;
static int H_MAX = 200;
static int S_MIN = 110;
static int S_MAX = 256;
static int V_MIN = 94;
static int V_MAX = 256;

static const int FRAME_WIDTH = 640;
static const int FRAME_HEIGHT = 480;

//const int MIN_OBJECT_AREA = 20*20;
//const int MAX_OBJECT_AREA = FRAME_HEIGHT*FRAME_WIDTH/1.5;

static const string windowName = "Original Image";
static const string windowName1 = "HSV Image";
static const string windowName2 = "Thresholded Image";
static const string windowName3 = "After Morphological Operations";

void computeAllMatchings(size_t index,
						 size_t n,
						 std::set<size_t> &used,
						 std::vector<size_t> &tmp,
						 float currentCost,
						 std::vector<size_t> &bestMatch,
						 const std::map<std::pair<size_t, size_t>, float> &costs,
						 float &bestCost) {
	assert(index <= n);
	if (currentCost >= bestCost) {
		return;
	} else if (index == tmp.size()) {
		bestMatch = tmp;
		bestCost = currentCost;
	} else {
		for (size_t i = 0; i < n; ++i) {
			if (used.find(i) == used.end()) {
				tmp[index] = i;
				used.insert(i);
				currentCost += (costs.find(std::make_pair(index, i)))->second;
				computeAllMatchings(index+1, n, used, tmp, currentCost, bestMatch, costs, bestCost);
			}
		}
	}
}

// computes the best assignment between A=[0..m] and B=[0..n],
// costs[(i,j)] is the cost of pairing A[i] with B[j]
std::vector<size_t> bestMatching(size_t m, size_t n, const std::map<std::pair<size_t, size_t>, float> &costs) {
	assert(m <= n);
	std::vector<size_t> bestMatch;
	if (m > 0 && n > 0) {
		float bestCost = std::numeric_limits<float>::max();
		for (size_t i = 0; i < n; ++i) {
			std::vector<size_t> tmp(m);
			std::set<size_t> used;
			tmp[0] = i;
			used.insert(i);
			computeAllMatchings(1, n, used, tmp, (costs.find(std::make_pair(0u,i)))->second, bestMatch, costs, bestCost);
		}
	}
	return bestMatch;
}

//computes the optimal pairings idx_prediction -> idx_object detected
std::vector< std::pair<size_t,size_t> > ComputeMatching(const std::vector<cv::Point2i> &predictions,
		const std::vector<cv::Point2i> &detections) {
	size_t smin = std::min(predictions.size(), detections.size());
	size_t smax = std::max(predictions.size(), detections.size());
	bool invert = (predictions.size() > detections.size()); /* if we have more predictions
	swap to invoke the enumeration, and then swap back at the end */
	std::map<std::pair<size_t, size_t>, float> costs;
	for(size_t i = 0; i < smin; ++i) {
		for (size_t j = 0; j < smax; ++j) {
			float x = invert ? float(predictions[j].x - detections[i].x) : float(predictions[i].x - detections[j].x);
			float y = invert ? float(predictions[j].y - detections[i].y) : float(predictions[i].y - detections[j].y);
			costs[std::make_pair(i,j)] = std::sqrt(x*x + y*y);
		}
	}
	std::vector<size_t> pairing = bestMatching(smin, smax, costs);
	std::vector< std::pair<size_t,size_t> > matching;
	for (size_t i = 0; i < pairing.size(); ++i) {
		if (invert) {
			//bestMatching returned a list of [idx_detection, idx_prediction]
			matching.push_back(std::make_pair(pairing[i], i));
		} else {
			//bestMatching returned a list of [idx_prediction, idx_detection]
			matching.push_back(std::make_pair(i, pairing[i]));
		}
	}
}

cv::Point2f transformPoint(cv::Point2f src, cv::Mat transform) {
	std::cout << "REFACTOR ME" << std::endl;
	std::vector<double> tmp(3);
	tmp[0] = src.x; tmp[1] = src.y; tmp[2] = 1.0;

	cv::Mat pt(tmp);
	cv::Mat dst = transform * pt;

	return cv::Point2f(dst.at<double>(0,0) / dst.at<double>(0,2),
					   dst.at<double>(0,1) / dst.at<double>(0,2));
}

void *tracker2(void *arg) {

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
	//cam.set(CV_CAP_PROP_AUTO_EXPOSURE, 0.0);
	//cam.set(CV_CAP_PROP_EXPOSURE, 157.0);
	//cout << cam.get(CV_CAP_PROP_AUTO_EXPOSURE) << " " << cam.get(CV_CAP_PROP_EXPOSURE) << " **\n";
#endif

	if (!cam.isOpened()) {
		cerr << "Failed to open the camera" << endl;
		return 0;
	}

	cv::Mat img2world;
	cv::FileStorage fs(rhs::PathToCalibrationData, cv::FileStorage::READ);
	if (!fs.isOpened() || fs[rhs::PerspectiveTransformationName].type() == cv::FileNode::NONE) {
		//disaster
		std::cerr << "WARNING: Unable to read calibration data from " << rhs::PathToCalibrationData
				  << ", using image coordinates" << std::endl;
		img2world = cv::Mat(3, 3, CV_32F);
		cv::setIdentity(img2world);
	} else {
		fs[rhs::PerspectiveTransformationName] >> img2world;
	}
	fs.release();

	cv::Mat image;
	std::vector< std::vector<cv::Point> > contours;
	int key_code;
	rhs::Timer timer;

	// First detection to initialize the control structures
	cam.read(image);
	rhs::ColorBasedDetector detector(cv::Scalar(H_MIN,S_MIN,V_MIN), cv::Scalar(H_MAX,S_MAX,V_MAX));
	detector.detectObjects(image, contours);
	// For each object set up a kalman filter
	std::vector<rhs::MovingObject> objects;
	int objectCount = 0;
	for (size_t i = 0; i < contours.size(); ++i) {
		cv::Rect bbox = cv::boundingRect(contours[i]);
		float x = bbox.x + bbox.width / 2.0f;
		float y = bbox.y + bbox.height / 2.0f;
		std::stringstream ss;
		ss << "obj" << (objectCount++);
		objects.push_back(rhs::MovingObject(ss.str(), x, y));
	}

	float dt;
	cv::Mat measurement(2, 1, CV_32F);
	int font = cv::FONT_HERSHEY_PLAIN;
    double font_scale = 0.8;
	while (tracking) {
		// Grab a frame
		cam.grab();
		dt = timer.timeElapsed();
		timer.restart();
		cam.retrieve(image);

		detector.detectObjects(image, contours);
		std::vector<cv::Point2i> predictions; // coupled with the objects array
		for (size_t i = 0; i < objects.size(); ++i) {
			predictions.push_back(objects[i].predictPosition(dt));
		}
		std::vector<cv::Point2i> detections;
		for (size_t i = 0; i < contours.size(); ++i) {
			cv::Rect b = cv::boundingRect(contours[i]);
			detections.push_back(cv::Point2i(b.x + b.width / 2.0f, b.y + b.height / 2.0f));


			std::cout << "REMOVE ME!\n";
			cv::Point2f groundPoint = transformPoint(detections.back(), img2world);
			std::stringstream xWorld, yWorld;
			xWorld << groundPoint.x;
			yWorld << groundPoint.y;
			cv::putText(image, xWorld.str(), detections.back(), font, font_scale, cv::Scalar(0,255,0));
			cv::putText(image, yWorld.str(), detections.back()+cv::Point2i(0,10), font, font_scale, cv::Scalar(0,255,0));

		}
		// perform the pairwise matching between the identified contours and the
		// MovingObjects currently tracked by using either the hungarian algorithm
		// or (assuming we are tracking few objects) by checking every combination
		// that minimizes the sum of the distances
		/*std::vector< std::pair<size_t,size_t> > matching = ComputeMatching(predictions, detections);
		std::vector<cv::Point2i> used;
		for (size_t i = 0; i < matching.size(); ++i) {
			rhs::MovingObject &tracker = objects[matching[i].first];
			cv::Point2i match = detections[matching[i].second];

			cv::Point2f groundPoint = transformPoint(match, img2world);
			cv::putText(image, tracker.tag(), cv::Point2i(match.x,match.y), font, font_scale, cv::Scalar(0,255,0));
			std::stringstream xWorld, yWorld;
			xWorld << groundPoint.x;
			yWorld << groundPoint.y;
			cv::putText(image, xWorld.str(), cv::Point2i(match.x,match.y+10), font, font_scale, cv::Scalar(0,255,0));
			cv::putText(image, yWorld.str(), cv::Point2i(match.x,match.y+20), font, font_scale, cv::Scalar(0,255,0));

			measurement.at<float>(0) = match.x;
			measurement.at<float>(1) = match.y;
			used.push_back(match);
			tracker.feedback(measurement);
		}
		//remove used detections (if any)
		for (size_t i = 0; i < used.size(); ++i) {
			detections.erase(std::remove(detections.begin(), detections.end(), used[i]), detections.end());
		}
		//set up new trackers for unmatched detections (if any)
		for (size_t i = 0; i < detections.size(); ++i) {
			std::stringstream ss;
			ss << "obj" << (objectCount++);
			objects.push_back(rhs::MovingObject(ss.str(), detections[i].x, detections[i].y));
		}*/

		/* plot points
		#define drawCross( center, color, d )                                 \
			cv::line( image, cv::Point( center.x - d, center.y - d ),                \
						 cv::Point( center.x + d, center.y + d ), color, 1, CV_AA, 0); \
			cv::line( image, cv::Point( center.x + d, center.y - d ),                \
						 cv::Point( center.x - d, center.y + d ), color, 1, CV_AA, 0 )
		drawCross(predictedPoint, cv::Scalar(255, 120, 0), 5);*/

		cv::imshow(windowName, image);

		// TODO if trackers lost their object for more than a given threshold, remove them
		objects.erase(std::remove_if(objects.begin(), objects.end(), rhs::outdated(2.0f)), objects.end());

		key_code = cv::waitKey(1);
		if (key_code == 32 || (key_code & 0xff) == 32 ) {
			// skip
		}
	}

	cam.release();
	cv::destroyAllWindows();

	return 0;
}
