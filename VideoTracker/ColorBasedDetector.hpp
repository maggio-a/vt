#ifndef RHS_COLORBASEDDETECTOR_HDR
#define RHS_COLORBASEDDETECTOR_HDR

#include "IDetector.hpp"

#include <vector>
#include <opencv2/opencv.hpp>

namespace rhs {

class ColorBasedDetector : public IDetector {
public:
	ColorBasedDetector(cv::Scalar HSVMin, cv::Scalar HSVMax);
	~ColorBasedDetector();

	void DetectObjects(const cv::Mat &image, std::vector< std::vector<cv::Point2i> > &contours_out);

private:
	cv::Scalar rangeMin;
	cv::Scalar rangeMax;
	cv::Mat hsv;
	cv::Mat mask;
};

ColorBasedDetector::ColorBasedDetector(cv::Scalar HSVMin, cv::Scalar HSVMax)
		: rangeMin(HSVMin), rangeMax(HSVMax), hsv(), mask() {

}

ColorBasedDetector::~ColorBasedDetector() {

}

void ColorBasedDetector::DetectObjects(const cv::Mat &image, std::vector< std::vector<cv::Point2i> > &contours_out) {
	cv::cvtColor(image, hsv, CV_BGR2HSV);
    cv::inRange(hsv, rangeMin, rangeMax, mask);
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_RECT,cv::Size(5,5)));
    //cv::findContours(tmp, cnt, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
    cv::findContours(mask, contours_out, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
}

} // rhs namespace

#endif
