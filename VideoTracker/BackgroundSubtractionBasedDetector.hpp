#ifndef RHS_BGSBASEDDETECTOR_HDR
#define RHS_BGSBASEDDETECTOR_HDR

#include "IDetector.hpp"

#include <vector>
#include <opencv2/opencv.hpp>

class BackgroundSubtractionBasedDetector : public IDetector {
public:
	BackgroundSubtractionBasedDetector();
	~BackgroundSubtractionBasedDetector();

	void DetectObjects(const cv::Mat &image, std::vector< std::vector<cv::Point2i> > &contours_out);

private:
	cv::BackgroundSubtractorMOG bgs;
	cv::Mat mask;
};

// FIXME parameters
BackgroundSubtractionBasedDetector::BackgroundSubtractionBasedDetector()
		: bgs(10, 3, 0.6, 20), fgmask() {

}

BackgroundSubtractionBasedDetector::~BackgroundSubtractionBasedDetector() {

}

void BackgroundSubtractionBasedDetector::DetectObjects(
		const cv::Mat &image, std::vector< std::vector<cv::Point2i> > &contours_out) {
	cv::Mat snapshot;
	image.copyTo(snapshot);
	cv::medianBlur(snapshot, snapshot, 5);
	bgs(snapshot, mask);
	cv::morphologyEx(mask, mask, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_RECT,cv::Size(5,5)));
	cv::findContours(mask, contours_out, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
}

#endif
