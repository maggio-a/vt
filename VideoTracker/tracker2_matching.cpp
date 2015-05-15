#include <iostream>
#include <string>
#include <cstdio>
#include <memory>
#include <opencv2/opencv.hpp>
#ifdef __arm__
  #include <raspicam/raspicam_cv.h>
#endif

#include "BackgroundSubtractionBasedDetector.hpp"
#include "ColorBasedDetector.hpp"
#include "MovingObject.hpp"
#include "Timer.hpp"
#include "calibration.hpp"
#include "hungarian.hpp"

extern bool tracking;
extern std::auto_ptr<Socket> channel;

using namespace std;
using namespace cv;


static int H_MIN = 0;
static int H_MAX = 200;
static int S_MIN = 110;
static int S_MAX = 256;
static int V_MIN = 94;
static int V_MAX = 256;

static const int FRAME_WIDTH = 320;
static const int FRAME_HEIGHT = 240;

//const int MIN_OBJECT_AREA = 20*20;
//const int MAX_OBJECT_AREA = FRAME_HEIGHT*FRAME_WIDTH/1.5;

static const string windowName = "Original Image";
static const string windowName1 = "HSV Image";
static const string windowName2 = "Thresholded Image";
static const string windowName3 = "After Morphological Operations";


Point2f transformPoint(Point2f src, Mat transform) {
	cout << "REFACTOR ME" << endl;
	vector<double> tmp(3);
	tmp[0] = src.x; tmp[1] = src.y; tmp[2] = 1.0;

	Mat pt(tmp);
	Mat dst = transform * pt;

	return Point2f(dst.at<double>(0,0) / dst.at<double>(0,2),
					   dst.at<double>(0,1) / dst.at<double>(0,2));
}

void *tracker2(void *arg) {

#ifdef __arm__
	raspicam::RaspiCam_Cv cam;
	cam.set(CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
	cam.set(CV_CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);
	cam.open();
#else
	VideoCapture cam(0);
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

	Mat img2world;
	FileStorage fs(rhs::PathToCalibrationData, FileStorage::READ);
	if (!fs.isOpened() || fs[rhs::PerspectiveTransformationName].type() == FileNode::NONE) {
		//disaster
		cerr << "WARNING: Unable to read calibration data from " << rhs::PathToCalibrationData
				  << ", using plain image coordinates" << endl;
		img2world = Mat(3, 3, CV_64F);
		setIdentity(img2world);
	} else {
		fs[rhs::PerspectiveTransformationName] >> img2world;
	}
	fs.release();

	Mat image;
	// warm up camera
	for (size_t i = 0; i < 10; ++i) {
		cam.read(image);
	}
	vector< vector<Point2i> > contours;
	int key_code;
	rhs::Timer timer;

	// First detection to initialize the control structures
	cam.read(image);
	//rhs::ColorBasedDetector detector(Scalar(H_MIN,S_MIN,V_MIN), Scalar(H_MAX,S_MAX,V_MAX));
	rhs::BackgroundSubtractionBasedDetector detector;
	detector.DetectObjects(image, contours);
	// For each object set up a kalman filter
	vector<rhs::MovingObject> objects;
	int objectCount = 0;
	for (size_t i = 0; i < contours.size(); ++i) {
		Rect bbox = boundingRect(contours[i]);
		float x = bbox.x + bbox.width / 2.0f;
		float y = bbox.y + bbox.height / 2.0f;
		stringstream ss;
		ss << "OBJ   " << (objectCount++);
		objects.push_back(rhs::MovingObject(ss.str(), x, y));
	}

	float dt;
	Mat measurement(2, 1, CV_32F);
	int font = FONT_HERSHEY_PLAIN;
    double font_scale = 1.0;
    int c = 0;
	while (tracking) {
		c++;
		// Grab a frame
		cam.grab();
		dt = timer.timeElapsed();
		timer.restart();
		cam.retrieve(image);

		cout << dt << endl;

		detector.DetectObjects(image, contours);
		vector<Point2f> predictions; // coupled with the objects array
		for (size_t i = 0; i < objects.size(); ++i) {
			predictions.push_back(objects[i].predictPosition(dt));
		}
		vector<Point2f> detections;
		for (size_t j = 0; j < contours.size(); ++j) {
			Rect b = boundingRect(contours[j]);
			detections.push_back(Point2f(b.x + b.width / 2.0f, b.y + b.height / 2.0f));
		}
		// perform the pairwise matching between the identified contours and the
		// MovingObjects currently tracked by using either the hungarian algorithm
		// or (assuming we are tracking few objects) by checking every combination
		// that minimizes the sum of the distances

		// FIXME fix this, now only prints predictions if we have any detection i should probably print the statePost of the filters	
		if (detections.size() > 0) { // if we detected objects
			set<size_t> used;
			if (predictions.size() > 0) { // if already tracking objects, compute the matching 
				vector<size_t> matching = ComputeMatching(predictions, detections);
				for (size_t i = 0; i < matching.size(); ++i) {
					rhs::MovingObject &tracker = objects[i];
					size_t j = matching[i];
					if (j < detections.size()) {
						Point2f match = detections[j];
						Point2f groundPoint = transformPoint(match, img2world);

		#define drawCross( center, color, d )                                 \
			cv::line( image, cv::Point( center.x - d, center.y - d ),                \
						 cv::Point( center.x + d, center.y + d ), color, 1, CV_AA, 0); \
			cv::line( image, cv::Point( center.x + d, center.y - d ),                \
						 cv::Point( center.x - d, center.y + d ), color, 1, CV_AA, 0 )
		drawCross(Point2i(match.x,match.y), cv::Scalar(255, 120, 0), 5);



						putText(image, tracker.tag(), Point2i(match.x,match.y), font, font_scale, Scalar(0,255,0));
						stringstream xWorld, yWorld;
						xWorld << "xw: " << groundPoint.x;
						yWorld << "yw: " << groundPoint.y;
						putText(image, xWorld.str(), Point2i(match.x,match.y+15), font, font_scale, Scalar(0,255,0));
						putText(image, yWorld.str(), Point2i(match.x,match.y+30), font, font_scale, Scalar(0,255,0));
						
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
					stringstream ss;
					ss << "OBJ   " << (objectCount++);
					objects.push_back(rhs::MovingObject(ss.str(), detections[j].x, detections[j].y));
				}
			}
		}

		imshow(windowName, image);
		imshow("BGS", detector.maskout);
		/*if (c > 150 && c % 10) {
			stringstream ss; ss << "snapshot" << c << ".jpg";
			imwrite(ss.str(), image);
		}*/

		// if trackers lost their object for more than a given threshold, remove them
		objects.erase(remove_if(objects.begin(), objects.end(), rhs::outdated(2.0f)), objects.end());

		key_code = waitKey(1);
		if (key_code == 32 || (key_code & 0xff) == 32 ) {
			// skip
		}
	}

	cam.release();
	cv::destroyAllWindows();

	return 0;
}
