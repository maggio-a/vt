#include <iostream>
#include <string>
#include <memory>
#include <opencv2/opencv.hpp>
#ifdef __arm__
  #include <raspicam/raspicam_cv.h>
#endif

#include "Socket.hpp"
#include "BackgroundSubtractionBasedDetector.hpp"
#include "ColorBasedDetector.hpp"
#include "MovingObject.hpp"
#include "Timer.hpp"
#include "Snapshot.hpp"
#include "calibration.hpp"


#include <sstream>

extern bool tracking;
extern std::auto_ptr<Socket> channel;
extern rhs::Timer live;

using namespace std;
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


Point2f transformPoint(Point2f src, const Mat &transform) {
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
	// warm up camera (2 seconds)
	rhs::Timer tlocal;
	while(tlocal.timeElapsed() < 2.0f) {
		cam.grab();
	}
	vector< vector<Point2i> > contours;

	rhs::ColorBasedDetector detector(Scalar(H_MIN,S_MIN,V_MIN), Scalar(H_MAX,S_MAX,V_MAX));
	//rhs::BackgroundSubtractionBasedDetector detector;

	float timestamp;
	int c=1;
	tlocal.restart();
	while (tracking) {
		cam.grab();
		timestamp = live.timeElapsed();
		cam.retrieve(image);

		detector.DetectObjects(image, contours);

		rhs::Snapshot snap(timestamp);
		for (size_t j = 0; j < contours.size(); ++j) {
			Rect b = boundingRect(contours[j]);
			Point2f imgPoint(b.x + b.width / 2.0f, b.y + b.height / 2.0f);
			Point2d groundPoint = transformPoint(imgPoint, img2world);
			snap.addObject(groundPoint);
		}

		rhs::Message msg(rhs::OBJECT_DATA, snap.str());
		channel->Send(msg);

		c++;

		/*imshow("mask", detector.maskout);
		int keyCode = waitKey(10);
		if (keyCode == ' ' || (keyCode & 0xff) == ' ') {
			stringstream ss;
			ss << "shot" << c++ << ".png";
			imwrite(ss.str(), image);
		}*/
	}

	cout << "Retrieved " << c << " frames in " << tlocal.timeElapsed() << " seconds.\n";

	cam.release();

	return 0;
}
