// =============================================================================
//
//  This file is part of the final project source code for the course "Ad hoc
//  and sensor networks" (Master's degree in Computer Science, University of
//  Pisa)
//
//  Copyright (C) 2015, Andrea Maggiordomo
//
// =============================================================================

#ifndef RHS_COLORBASEDDETECTOR_HDR
#define RHS_COLORBASEDDETECTOR_HDR

#include "IDetector.hpp"

#include <vector>
#include <opencv2/opencv.hpp>

namespace rhs {

// class ColorBasedDetector
// Performs object detection using color filtering. It works by converting the original image
// in the HSV color space and performing a range test on the pixel values
class ColorBasedDetector : public IDetector {
public:

	// Constructor
	// HSVMin and HSVMax are triplets determining the minimum and maximum values for each component
	// (Hue, Saturation and Value) once the image gets converted to the HSV color space
	ColorBasedDetector(cv::Scalar HSVMin, cv::Scalar HSVMax) 
			: rangeMin(HSVMin), rangeMax(HSVMax), hsv(), mask() {
	}

	~ColorBasedDetector() {  }

	// Implementation of the IDetector interface
	void DetectObjects(const cv::Mat &image, std::vector< std::vector<cv::Point2i> > &contours_out) {
		cv::cvtColor(image, hsv, CV_BGR2HSV);
		cv::inRange(hsv, rangeMin, rangeMax, mask);
		cv::morphologyEx(mask, mask, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_RECT,cv::Size(5,5)));
		cv::findContours(mask, contours_out, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
	}

private:
	cv::Scalar rangeMin;
	cv::Scalar rangeMax;
	cv::Mat hsv;
	cv::Mat mask;
};

} // rhs namespace

#endif
