#ifndef RHS_IDETECTOR_HDR
#define RHS_IDETECTOR_HDR

#include <vector>
#include <opencv2/core/core.hpp>

namespace rhs {

class IDetector {
public:
	virtual void DetectObjects(const cv::Mat &image, std::vector< std::vector<cv::Point2i> > &contours_out) = 0;
	virtual ~IDetector();
};

} // rhs namespace

#endif