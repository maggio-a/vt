#ifndef RHS_BGSBASEDDETECTOR_HDR
#define RHS_BGSBASEDDETECTOR_HDR

#include "IDetector.hpp"

#include <vector>
#include <opencv2/opencv.hpp>

namespace rhs {

class BackgroundSubtractionBasedDetector : public IDetector {
public:
	cv::Mat maskout;

	BackgroundSubtractionBasedDetector(int history, double threshold, int morph_x, int morph_y, float learn=0.02, bool shadows = false)
			: maskout(), bgs(history, threshold, shadows), mx(morph_x), my(morph_y), lr(learn), mask() {
	}
	
	~BackgroundSubtractionBasedDetector() { }

	void DetectObjects(const cv::Mat &image, std::vector< std::vector<cv::Point2i> > &contours_out) {
		cv::Mat still;
		image.copyTo(still);
		bgs(still, mask, lr);
		cv::morphologyEx(mask, mask, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_RECT,cv::Size(mx,my)));
		mask.copyTo(maskout);
		cv::findContours(mask, contours_out, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
	}

private:
	cv::BackgroundSubtractorMOG2 bgs;
	int mx;
	int my;
	float lr;
	cv::Mat mask;
};

} // rhs namespace

#endif
