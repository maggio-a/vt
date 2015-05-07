#include <iostream>
#include <string>
#include <stdio.h>
#include <time.h>
#include <opencv2/opencv.hpp>

#ifdef __arm__

#include <raspicam/raspicam_cv.h>

#endif

#include "ColorBasedDetector.hpp"
#include "MovingObject.hpp"

extern bool tracking;

using std::cout;
using std::cerr;
using std::endl;
using std::string;


static int H_MIN = 0;
static int H_MAX = 188;
static int S_MIN = 96;
static int S_MAX = 195;
static int V_MIN = 143;
static int V_MAX = 256;

static const int FRAME_WIDTH = 1280;
static const int FRAME_HEIGHT = 720;

//const int MIN_OBJECT_AREA = 20*20;
//const int MAX_OBJECT_AREA = FRAME_HEIGHT*FRAME_WIDTH/1.5;

static const string windowName = "Original Image";
static const string windowName1 = "HSV Image";
static const string windowName2 = "Thresholded Image";
static const string windowName3 = "After Morphological Operations";
static const string trackbarWindowName = "Trackbars";

/* Assumes t2 > t1 */
static inline double getTimeDelta(const struct timespec &t2, const struct timespec &t1) {
	return ((t2.tv_sec*1000.0 + t2.tv_nsec/1000000.0) - (t1.tv_sec*1000.0 + t1.tv_nsec/1000000.0)) / 1000.0;
}




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
	float bestCost = std::numeric_limits<float>::max();
	for (size_t i = 0; i < n; ++i) {
		std::vector<size_t> tmp(m);
		std::set<size_t> used;
		tmp[0] = i;
		used.insert(i);
		computeAllMatchings(1, n, used, tmp, (costs.find(std::make_pair(0u,i)))->second, bestMatch, costs, bestCost);
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

	cv::Mat image;

	if (!cam.isOpened()) {
		cerr << "Failed to open the camera" << endl;
		return 0;
	}

	std::vector< std::vector<cv::Point> > contours;

	int key_code;

	struct timespec t_prev, t_curr;


	clock_gettime(CLOCK_MONOTONIC, &t_prev);

	// First detection to initialize the control structures
	cam.read(image);
	ColorBasedDetector detector(cv::Scalar(H_MIN,S_MIN,V_MIN), cv::Scalar(H_MAX,S_MAX,V_MAX));
	detector.detectObjects(image, contours);
	// For each object set up a kalman filter
	std::vector<MovingObject> objects;
	int objectCount = 0;
	for (size_t i = 0; i < contours.size(); ++i) {
		cv::Rect bbox = cv::boundingRect(contours[i]);
		float x = bbox.x + bbox.width / 2.0f;
		float y = bbox.y + bbox.height / 2.0f;
		std::stringstream ss;
		ss << "obj" << (objectCount++);
		objects.push_back(MovingObject(ss.str(), x, y));
	}

	float dt;
	cv::Mat measurement(2, 1, CV_32F);
	while (tracking) {
		// Grab a frame
		cam.grab();
		clock_gettime(CLOCK_MONOTONIC, &t_curr);
		dt = float(getTimeDelta(t_curr, t_prev));
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
		}
		// perform the pairwise matching between the identified contours and the
		// MovingObjects currently tracked by using either the hungarian algorithm
		// or (assuming we are tracking few objects) by checking every combination
		// that minimizes the sum of the distances
		std::vector< std::pair<size_t,size_t> > matching = ComputeMatching(predictions, detections);
		std::vector<cv::Point2i> used;
		for (size_t i = 0; i < matching.size(); ++i) {
			MovingObject &tracker = objects[matching[i].first];
			cv::Point2i match = detections[matching[i].second];
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
			objects.push_back(MovingObject(ss.str(), detections[i].x, detections[i].y));
		}

		/* plot points
		#define drawCross( center, color, d )                                 \
			cv::line( image, cv::Point( center.x - d, center.y - d ),                \
						 cv::Point( center.x + d, center.y + d ), color, 1, CV_AA, 0); \
			cv::line( image, cv::Point( center.x + d, center.y - d ),                \
						 cv::Point( center.x - d, center.y + d ), color, 1, CV_AA, 0 )
		drawCross(predictedPoint, cv::Scalar(255, 120, 0), 5);*/

		cv::imshow(windowName, image);

		// TODO if trackers lost their object for more than a given threshold, remove them

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
/*
std::vector< std::pair<size_t,size_t> > _ComputeMatching(
		const std::vector<cv::Point2i> &predictions,
		const std::vector<std::vector< cv::Point2i> > &detectedContours) {
	size_t n1 = mo.size(), n2 = cntrs.size();
	assert(n1 > 0 && n2 > 0);
	std::vector< std::pair<size_t,size_t> > pairs;
	for(size_t i = 0; i < n1; ++i) {
		for (size_t j = 0; j < n2; ++j) {
			pairs.push_back(make_pair(i,j));
		}
	}
	cout << "FIXME FIXME FIXME puo prendere piu volte lo stesso elemento di un array";
	// lo implemento ricorsivo e fu
	size_t nmatches = std::max(n1, n2);
	std::vector< std::pair<size_t,size_t> > matching(nmatches);
	std::vector<bool> selected(pairs.size(), false);
	fill_n(selected, nmatches, true);
	float cost = numeric_limits<float>::max();
	do {
		//compute cost and save the best found
	} while (next_premutation(pairs.begin(), pairs.end()));
}*/
