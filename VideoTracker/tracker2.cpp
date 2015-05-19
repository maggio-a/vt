#include <iostream>
#include <string>
#include <sstream>
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
extern rhs::CameraParams params;

using namespace std;
using namespace cv;

//const int MIN_OBJECT_AREA = 20*20;
//const int MAX_OBJECT_AREA = FRAME_HEIGHT*FRAME_WIDTH/1.5;


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
	cam.set(CV_CAP_PROP_FRAME_WIDTH, params.resWidth);
	cam.set(CV_CAP_PROP_FRAME_HEIGHT, params.resHeight);

	if (params.shutterSpeed != -1) cam.set(CV_CAP_PROP_EXPOSURE, params.shutterSpeed);
	if (params.brightness != -1) cam.set(CV_CAP_PROP_BRIGHTNESS, params.brightness);
	if (params.saturation != -1) cam.set(CV_CAP_PROP_SATURATION, params.saturation);
	if (params.contrast != -1) cam.set(CV_CAP_PROP_CONTRAST, params.contrast);
	if (params.gain != -1) cam.set(CV_CAP_PROP_GAIN, params.gain);

  #if CV_MINOR_VERSION == 4 && CV_SUBMINOR_VERSION == 11
	if (params.wb_b != -1) cam.set(CV_CAP_PROP_WHITE_BALANCE_U, params.wb_b);
	if (params.wb_r != -1) cam.set(CV_CAP_PROP_WHITE_BALANCE_V,  params.wb_r);
  #else
	if (params.wb_r != -1) cam.set(CV_CAP_PROP_WHITE_BALANCE_RED_V,  params.wb_r);
	if (params.wb_b != -1) cam.set(CV_CAP_PROP_WHITE_BALANCE_BLUE_U, params.wb_b);
  #endif
	
	cam.open();

#else

	VideoCapture cam(0);
	//cam.set(CV_CAP_PROP_FPS, 80.0);
	cam.set(CV_CAP_PROP_FRAME_WIDTH, params.resWidth);
	cam.set(CV_CAP_PROP_FRAME_HEIGHT, params.resHeight);

#endif

	if (!cam.isOpened()) {
		cerr << "Failed to open the camera" << endl;
		return 0;
	}

	Mat img2world;
	float groundWidth, groundHeight;
	FileStorage fs(rhs::PathToCalibrationData, FileStorage::READ);
	if (!fs.isOpened() || fs[rhs::PerspectiveTransformationName].type() == FileNode::NONE) {
		//disaster
		cerr << "WARNING: Unable to read calibration data from " << rhs::PathToCalibrationData
				  << ", using plain image coordinates" << endl;
		img2world = Mat(3, 3, CV_64F);
		setIdentity(img2world);
		groundWidth = params.frameWidth;
		groundHeight = params.frameHeight;
	} else {
		fs[rhs::PerspectiveTransformationName] >> img2world;
		fs[rhs::GroundWidthParamName] >> groundWidth;
		fs[rhs::GroundHeightParamName] >> groundHeight;
	}
	fs.release();

	stringstream ss;
	ss << groundWidth << " " << groundHeight;
	channel->Send(rhs::Message(rhs::STREAM_START, ss.str()));

	// warm up camera (3 seconds)
	rhs::Timer tlocal;
	while(tlocal.timeElapsed() < 3.0f) {
		cam.grab();
	}


	Mat capture;
	Mat rsz;
	bool resizeCapture = (params.resWidth != params.frameWidth) || (params.resHeight != params.frameHeight);
	vector< vector<Point2i> > contours;

	rhs::BackgroundSubtractionBasedDetector detector(params.bgsHistory, params.bgsThreshold, params.bgsMorphX, params.bgsMorphY, params.bgsLearningRate);

	float timestamp;
	int c=1;
	tlocal.restart();
	while (tracking) {
		cam.grab();
		timestamp = live.timeElapsed();
		cam.retrieve(capture);

		if (resizeCapture) {
			resize(capture, rsz, Size(params.frameWidth, params.frameHeight));
			detector.DetectObjects(rsz, contours);
		} else {
			detector.DetectObjects(capture, contours);
		}

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

	channel->Send(rhs::Message(rhs::STREAM_STOP));

	cout << "Retrieved " << c << " frames in " << tlocal.timeElapsed() << " seconds.\n";

	cam.release();

	return 0;
}
