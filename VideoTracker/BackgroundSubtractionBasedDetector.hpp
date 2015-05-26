#ifndef RHS_BGSBASEDDETECTOR_HDR
#define RHS_BGSBASEDDETECTOR_HDR

#include "IDetector.hpp"

#include <vector>
#include <opencv2/opencv.hpp>

namespace rhs {

// class BackgroundSubtractionBasedDetector
// Performs object detection detection using Background subtraction (see OpenCV documentation for some examples)
class BackgroundSubtractionBasedDetector : public IDetector {
public:
	cv::Mat maskout; // unused but useful for debug, after a call will show the detected foreground masks

	// Constructor
	// history, threshold and shadows are parameters of the underlying BackgroundSubtractor object (see OpenCV docs
	// for better explanations)
	//   - history:   last numbers of frames that are accounted for the background model
	//   - threshold: threshold to decide whether a pixel is described by the background or not
	//   - shadows:   performs shadows detection (shadow elements are marked with a different value
	//                in the foreground mask)
	// morph_x and morph_y are the dimensions of the probe using during morphological opening to get rid
	// of the noise.
	// learn is the learning rate, determines (during the background subtraction step) how fast new elements
	// are incorporated in the background.
	BackgroundSubtractionBasedDetector(int history, double threshold, int morph_x, int morph_y, float learn=0.02, bool shadows=false)
			: maskout(), bgs(history, threshold, shadows), mx(morph_x), my(morph_y), lr(learn), mask() {
	}
	
	~BackgroundSubtractionBasedDetector() { }

	// Implementation of the IDetector interface
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
