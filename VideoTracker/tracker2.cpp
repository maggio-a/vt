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
#include "hungarian.hpp"

extern bool tracking;

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::vector;

using namespace cv;


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
				  << ", using plain image coordinates" << std::endl;
		img2world = cv::Mat(3, 3, CV_64F);
		cv::setIdentity(img2world);
	} else {
		fs[rhs::PerspectiveTransformationName] >> img2world;
	}
	fs.release();

	cv::Mat image;
	// warm up camera
	for (size_t i = 0; i < 10; ++i) {
		cam.read(image);
	}
	std::vector< std::vector<cv::Point> > contours;
	int key_code;
	rhs::Timer timer;

	// First detection to initialize the control structures
	cam.read(image);
	cv::imwrite("firstframe.png", image);
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
		ss << "OBJ " << (objectCount++);
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
		std::vector<cv::Point2f> predictions; // coupled with the objects array
		for (size_t i = 0; i < objects.size(); ++i) {
			predictions.push_back(objects[i].predictPosition(dt));
		}
		std::vector<cv::Point2f> detections;
		for (size_t j = 0; j < contours.size(); ++j) {
			cv::Rect b = cv::boundingRect(contours[j]);
			detections.push_back(cv::Point2f(b.x + b.width / 2.0f, b.y + b.height / 2.0f));
		}
		// perform the pairwise matching between the identified contours and the
		// MovingObjects currently tracked by using either the hungarian algorithm
		// or (assuming we are tracking few objects) by checking every combination
		// that minimizes the sum of the distances

		// FIXME fix this, now only prints predictions if we have any detection
		if (detections.size() > 0) { // if we detected objects
			std::set<size_t> used;
			if (predictions.size() > 0) { // if already tracking objects, compute the matching 
				vector<size_t> matching = ComputeMatching(predictions, detections);
				for (size_t i = 0; i < matching.size(); ++i) {
					rhs::MovingObject &tracker = objects[i];
					size_t j = matching[i];
					if (j < detections.size()) {
						cv::Point2f match = detections[j];
						cv::Point2f groundPoint = transformPoint(match, img2world);
						cv::putText(image, tracker.tag(), cv::Point2i(match.x,match.y), font, font_scale, cv::Scalar(0,255,0));
						std::stringstream xWorld, yWorld;
						xWorld << "xw: " << groundPoint.x;
						yWorld << "yw: " << groundPoint.y;
						cv::putText(image, xWorld.str(), cv::Point2i(match.x,match.y+12), font, font_scale, cv::Scalar(0,255,0));
						cv::putText(image, yWorld.str(), cv::Point2i(match.x,match.y+24), font, font_scale, cv::Scalar(0,255,0));
						
						measurement.at<float>(0) = match.x;
						measurement.at<float>(1) = match.y;
						assert(used.insert(j).second == true);
						tracker.feedback(measurement);
					} else {
						cout << "FIXME object without detection" << endl;
					}
				}
			}
			// set up new trackers for unmatched detections (if any)
			for (size_t j = 0; j < detections.size(); ++j) {
				if (used.find(j) == used.end()) {
					std::stringstream ss;
					ss << "obj " << (objectCount++);
					objects.push_back(rhs::MovingObject(ss.str(), detections[j].x, detections[j].y));
				}
			}
		}

		/* plot points
		#define drawCross( center, color, d )                                 \
			cv::line( image, cv::Point( center.x - d, center.y - d ),                \
						 cv::Point( center.x + d, center.y + d ), color, 1, CV_AA, 0); \
			cv::line( image, cv::Point( center.x + d, center.y - d ),                \
						 cv::Point( center.x - d, center.y + d ), color, 1, CV_AA, 0 )
		drawCross(predictedPoint, cv::Scalar(255, 120, 0), 5);*/

		cv::imshow(windowName, image);

		// if trackers lost their object for more than a given threshold, remove them
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
