#ifndef RHS_CALIBRATION_HDR
#define RHS_CALIBRATION_HDR

#include <string>
#include <opencv2/opencv.hpp>

namespace rhs {
	void performCalibration(float width, float height);
	const std::string PathToCalibrationData = "homography.yaml";
	const std::string PerspectiveTransformationName = "perspectiveTransformMatrix";
	const std::string GroundWidthParamName  = "groundWidth";
	const std::string GroundHeightParamName = "groundHeight";

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
				if (fs["bgsLearningRate"].type() == cv::FileNode::INT) fs["bgsLearningRate"] >> bgsLearningRate;
			} else {
				std::cout << "CameraParams: using defaults\n";
			}
		}

	};
}

#endif