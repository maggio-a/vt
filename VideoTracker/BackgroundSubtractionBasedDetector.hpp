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
	//cv::BackgroundSubtractorMOG2 bgs;
	cv::BackgroundSubtractorMOG bgs;
	cv::Mat mask;
};

// FIXME parameters
BackgroundSubtractionBasedDetector::BackgroundSubtractionBasedDetector()
		//: bgs(2000, 36.0f, false), mask() {
		: bgs(10, 5, 0.8f), mask() {

}

BackgroundSubtractionBasedDetector::~BackgroundSubtractionBasedDetector() {

}

void BackgroundSubtractionBasedDetector::DetectObjects(
		const cv::Mat &image, std::vector< std::vector<cv::Point2i> > &contours_out) {
	cv::Mat snapshot;
	image.copyTo(snapshot);
	cv::medianBlur(snapshot, snapshot, 5);
	bgs(snapshot, mask);
	if (image.size().width >= 640 && image.size().height>= 480)
		cv::morphologyEx(mask, mask, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_RECT,cv::Size(5,5)));
	mask.copyTo(maskout);
	cv::findContours(mask, contours_out, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
}

} // rhs namespace

#endif
