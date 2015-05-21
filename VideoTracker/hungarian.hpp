#ifndef RHS_MATCHING_HDR
#define RHS_MATCHING_HDR

#include <vector>
#include <opencv2/core/core.hpp>

std::vector<size_t> ComputeMatching(const std::vector<cv::Point2f> predictions, const std::vector<cv::Point2f> detections, long distMax=5000);

#endif