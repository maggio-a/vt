#include <vector>

#include <opencv2/opencv.hpp>

class ColorBasedDetector {
public:
	ColorBasedDetector();
	~ColorBasedDetector();

	void detectObjects(const cv::Mat &image, cv::OutputArray contours);

private:
	cv::Scalar _rangeMin;
	cv::Scalar _rangeMax;
	cv::Mat _hsv;
	cv::Mat _mask;
};

ColorBasedDetector::ColorBasedDetector(cv::Scalar HSVMin, cv::Scalar HSVMax)
		: _rangeMin(HSVMin), _rangeMax(HSVMax), _hsv(), _mask() {

}

ColorBasedDetector::~ColorBasedDetector() {

}

void ColorBasedDetector::detectObjects(const cv::Mat &image, std::vector< std::vector<Point2i> > &contours_out) {
	cv::cvtColor(image, _hsv, CV_BGR2HSV);
    cv::inRange(_hsv, _rangeMin, _rangeMax, _mask);
    cv::morphologyEx(_mask, _mask, cv::MORPH_OPEN,
    	             cv::getStructuringElement(cv::MORPH_RECT,cv::Size(5,5)));
    //cv::findContours(tmp, cnt, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
    cv::findContours(_mask, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
}
