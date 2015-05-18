#ifndef RHS_BGSBASEDDETECTOR_HDR
#define RHS_BGSBASEDDETECTOR_HDR

#include "IDetector.hpp"

#include <vector>
#include <opencv2/opencv.hpp>

namespace rhs {

class BackgroundSubtractionBasedDetector : public IDetector {
public:
	BackgroundSubtractionBasedDetector();
	~BackgroundSubtractionBasedDetector();

	void DetectObjects(const cv::Mat &image, std::vector< std::vector<cv::Point2i> > &contours_out);

	cv::Mat maskout;

private:
	cv::BackgroundSubtractorMOG2 bgs;
	cv::Mat mask;
};

// FIXME parameters
BackgroundSubtractionBasedDetector::BackgroundSubtractionBasedDetector()
		: bgs(100, 16.0f, false), mask() {
	//bgs.set("nShadowDetection", 0);
}

BackgroundSubtractionBasedDetector::~BackgroundSubtractionBasedDetector() {

}

void BackgroundSubtractionBasedDetector::DetectObjects(
		const cv::Mat &image, std::vector< std::vector<cv::Point2i> > &contours_out) {
	cv::Mat snapshot;
	image.copyTo(snapshot);
	bgs(snapshot, mask);
	if (image.size().width >= 640 && image.size().height>= 480)
		cv::morphologyEx(mask, mask, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_RECT,cv::Size(5,5)));
	mask.copyTo(maskout);
	cv::findContours(mask, contours_out, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
}

} // rhs namespace

#endif
