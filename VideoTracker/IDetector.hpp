// =============================================================================
//
//  This file is part of the final project source code for the course "Ad hoc
//  and sensor networks" (Master's degree in Computer Science, University of
//  Pisa)
//
//  Copyright (C) 2015, Andrea Maggiordomo
//
// =============================================================================

#ifndef RHS_IDETECTOR_HDR
#define RHS_IDETECTOR_HDR

#include <vector>
#include <opencv2/core/core.hpp>

namespace rhs {

// interface IDetector
// Classes implementing this interface allow a user to extract from a specified image
// a vector of contours (in OpenCV contours are vectors of points) describing the silhouettes
// of the detected objects
class IDetector {
public:
	// Detects objects on the specified image. contours_out is a by-result parameter that 
	// after the call holds the detected contours
	virtual void DetectObjects(const cv::Mat &image, std::vector< std::vector<cv::Point2i> > &contours_out) = 0;

	// Virtual destructor
	virtual ~IDetector() { }
};

} // rhs namespace

#endif