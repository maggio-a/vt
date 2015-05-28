// =============================================================================
//
//  This file is part of the final project source code for the course "Ad hoc
//  and sensor networks" (Master's degree in Computer Science, University of
//  Pisa)
//
//  Copyright (C) 2015, Andrea Maggiordomo
//
// =============================================================================

#ifndef RHS_MATCHING_HDR
#define RHS_MATCHING_HDR

#include <vector>
#include <opencv2/core/core.hpp>

// function ComputeMatching
// Computes a matching between two sets of points using the hungarian method. Returns a vector M
// such that M[i] = j iff predictions[i] was matched with detections[j].
// If predictions.size() > detections.size(), fake detections are added to pad the cost matrix (this is
// needed by the implementation used, which assumes that every item of the first set can be matched with a
// distinct item of the second set). Matchings of predictions with these fake detections can be easily spotted
// by checking if M[i] >= detections.size()
std::vector<size_t> ComputeMatching(const std::vector<cv::Point2f> predictions, const std::vector<cv::Point2f> detections, long distMax=5000);

#endif
