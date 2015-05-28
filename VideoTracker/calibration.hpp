// =============================================================================
//
//  This file is part of the final project source code for the course "Ad hoc
//  and sensor networks" (Master's degree in Computer Science, University of
//  Pisa)
//
//  Copyright (C) 2015, Andrea Maggiordomo
//
// =============================================================================

#ifndef RHS_CALIBRATION_HDR
#define RHS_CALIBRATION_HDR

#include <string>
#include <opencv2/opencv.hpp>

namespace rhs {
	// function performCalibration
	// Allows the user to compute a homography from points of the image plane to points lying on the
	// ground plane. The user draws a quad by selecting the vertices whose coordinates on the ground
	// plane are (0,0), (width,0), (width,height), (0,height), and a perspective transformation is then computed.
	// Opens a window with the camera view, the key commands are:
	//    SPACE       toggles marking mode
	//    LEFT CLICK  to mark a vertex if marking mode is active
	//    r           removes last mark
	//    ENTER       returns, computing and saving the homography if 4 vertices are marked
	void PerformCalibration(float width, float height);


	const std::string PathToCalibrationData = "homography.yaml";
	const std::string PerspectiveTransformationName = "perspectiveTransformMatrix";
	const std::string GroundWidthParamName  = "groundWidth";
	const std::string GroundHeightParamName = "groundHeight";

	// Path to the file storing camera parameters
	const std::string CameraParamsPath = "../params/camera_params.yaml";

	const static int defaultResWidth =      320;
	const static int defaultResHeight =     240;
	const static int defaultFrameWidth =    320;
	const static int defaultFrameHeight =   240;
	const static int defaultShutterSpeed =   -1;
	const static int defaultBrightness =     -1;
	const static int defaultSaturation =     -1;
	const static int defaultContrast =       -1;
	const static int defaultGain =           -1;
	const static int defaultWhiteBalance_R = -1;
	const static int defaultWhiteBalance_B = -1;

	const static int defaultBgsHistory = 100;
	const static float defaultBgsThreshold = 16.0f;
	const static int defaultBgsMorphX = 3;
	const static int defaultBgsMorphY = 3;
	const static float defaultBgsLearningRate = 0.02f;

	// Camera parameters for the raspberry camera module, can be read from the file specified by rhs::CameraParamsPath
	// SENSOR
	//     rx    (integer): width of the captured image
	//     ry    (integer): height of the captured image
	//     fx    (integer): width of the used (scaled) image
	//     fy    (integer): height of the used (scaled) image
	//         NOTE: if rx != fx or ry != fy then the server works with a resized copy of the captured image 
	//     ss    (integer): shutter speed (range 0-100) from 0 to 33 msec
	//     br    (integer): brightness (range 0-100)
	//     sa    (integer): saturation (range 0-100)
	//     co    (integer): contrast (range 0-100)
	//     gain  (integer): gain (range 0-100)
	//     wb_r  (integer): white balance, red channel (not working as of now, range 0-100)
	//     wb_b  (integer): white balance, blue channel (not working as of now, range 0-100)
	//
	// BACKGROUND SUBTRACTION (see BackgroundSubtractionBasedDetector)
	//    bgsHistory      (integer)
	//    bgsThreshold    (float)
	//    bgsMorphX       (integer)
	//    bgsMorphY       (integer)
	//    bgsLearningRate (float) 
	struct CameraParams {
		int resWidth;
		int resHeight;
		int frameWidth;
		int frameHeight;
		int shutterSpeed;
		int brightness;
		int saturation;
		int contrast;
		int gain;
		int wb_r;
		int wb_b;

		int bgsHistory;
		float bgsThreshold;
		int bgsMorphX;
		int bgsMorphY;
		float bgsLearningRate;

		CameraParams(std::string cfgPath) : resWidth(defaultResWidth), resHeight(defaultResHeight),
				  frameWidth(defaultFrameWidth), frameHeight(defaultFrameHeight), shutterSpeed(defaultShutterSpeed),
				  brightness(defaultBrightness), saturation(defaultSaturation), contrast(defaultContrast), gain(defaultGain),
				  wb_r(defaultWhiteBalance_R), wb_b(defaultWhiteBalance_B),
				  bgsHistory(defaultBgsHistory), bgsThreshold(defaultBgsThreshold), bgsMorphX(defaultBgsMorphX), bgsMorphY(defaultBgsMorphY),
				  bgsLearningRate(defaultBgsLearningRate) {

			cv::FileStorage fs(cfgPath, cv::FileStorage::READ);
			if (fs.isOpened()) {
				if (fs["rx"].type() == cv::FileNode::INT) fs["rx"] >> resWidth;
				if (fs["ry"].type() == cv::FileNode::INT) fs["ry"] >> resHeight;
				if (fs["fx"].type() == cv::FileNode::INT) fs["fx"] >> frameWidth;
				if (fs["fy"].type() == cv::FileNode::INT) fs["fy"] >> frameHeight;
				if (fs["ss"].type() == cv::FileNode::INT) fs["ss"] >> shutterSpeed;
				if (fs["br"].type() == cv::FileNode::INT) fs["br"] >> brightness;
				if (fs["sa"].type() == cv::FileNode::INT) fs["sa"] >> saturation;
				if (fs["co"].type() == cv::FileNode::INT) fs["co"] >> contrast;
				if (fs["gain"].type() == cv::FileNode::INT) fs["gain"] >> gain;
				if (fs["wb_r"].type() == cv::FileNode::INT) fs["wb_r"] >> wb_r;
				if (fs["wb_b"].type() == cv::FileNode::INT) fs["wb_b"] >> wb_b;

				if (fs["bgsHistory"].type() == cv::FileNode::INT) fs["bgsHistory"] >> bgsHistory;
				if (fs["bgsThreshold"].type() == cv::FileNode::FLOAT) fs["bgsThreshold"] >> bgsThreshold;
				if (fs["bgsMorphX"].type() == cv::FileNode::INT) fs["bgsMorphX"] >> bgsMorphX;
				if (fs["bgsMorphY"].type() == cv::FileNode::INT) fs["bgsMorphY"] >> bgsMorphY;
				if (fs["bgsLearningRate"].type() == cv::FileNode::FLOAT) fs["bgsLearningRate"] >> bgsLearningRate;
			} else {
				std::cout << "CameraParams: using defaults\n";
			}
		}

	};
}

#endif