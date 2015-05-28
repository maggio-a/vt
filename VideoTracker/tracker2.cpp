// =============================================================================
//
//  This file is part of the final project source code for the course "Ad hoc
//  and sensor networks" (Master's degree in Computer Science, University of
//  Pisa)
//
//  Copyright (C) 2015, Andrea Maggiordomo
//
// =============================================================================

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
#include "Msg.hpp"
#include "calibration.hpp"


#include <sstream>

using namespace std;
using namespace cv;
using namespace rhs;


extern bool tracking;
extern socketHandle_t channel;
extern Timer live;
extern CameraParams params;


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
		// the homography from image to ground plane is unavailable
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
	try {
		channel->Send(Message(Message::STREAM_START, ss.str()));
	} catch(...) {
		return 0;
	}

	// warm up camera (3 seconds)
	Timer tlocal;
	while(tlocal.TimeElapsed() < 3.0f) {
		cam.grab();
	}


	Mat capture;
	Mat rsz;
	// check if resizing is required
	bool resizeCapture = (params.resWidth != params.frameWidth) || (params.resHeight != params.frameHeight);
	vector< vector<Point2i> > contours;

	BackgroundSubtractionBasedDetector detector(params.bgsHistory, params.bgsThreshold, params.bgsMorphX, params.bgsMorphY, params.bgsLearningRate);

	float timestamp;
	int c=1;
	tlocal.Restart();
	while (tracking) {
		// capture
		cam.grab();
		timestamp = live.TimeElapsed();
		cam.retrieve(capture);

		if (resizeCapture) {
			resize(capture, rsz, Size(params.frameWidth, params.frameHeight));
			detector.DetectObjects(rsz, contours);
		} else {
			detector.DetectObjects(capture, contours);
		}

		// create the snapshot
		Snapshot snap(timestamp);
		for (size_t j = 0; j < contours.size(); ++j) {
			Rect b = boundingRect(contours[j]);
			Point2f imgPoint(b.x + b.width / 2.0f, b.y + b.height / 2.0f);
			Point2d groundPoint = transformPoint(imgPoint, img2world);
			snap.AddObject(groundPoint);
		}

		// send it to the client
		Message msg(Message::OBJECT_DATA, snap.str());
		try {
			channel->Send(msg);
		} catch (...) {
			return 0;
		}

		c++;

		/*imshow("mask", detector.maskout);
		int keyCode = waitKey(10);
		if (keyCode == ' ' || (keyCode & 0xff) == ' ') {
			stringstream ss;
			ss << "shot" << c++ << ".png";
			imwrite(ss.str(), image);
		}*/
	}

	try {
		channel->Send(Message(Message::STREAM_STOP));
	} catch (...) {
		return 0;
	}

	cout << "Retrieved " << c << " frames in " << tlocal.TimeElapsed() << " seconds.\n";

	cam.release();

	return 0;
}
