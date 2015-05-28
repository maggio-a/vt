// =============================================================================
//
//  This file is part of the final project source code for the course "Ad hoc
//  and sensor networks" (Master's degree in Computer Science, University of
//  Pisa)
//
//  Copyright (C) 2015, Andrea Maggiordomo
//
// =============================================================================

#ifndef RHS_SNAPSHOT_HDR
#define RHS_SNAPSHOT_HDR

#include <string>
#include <opencv2/core/core.hpp>

namespace rhs {

// class Snapshot
// This class represents the data extracted from an image capture by a server. It contains a timestamp
// of the capture time and a vector of positions (encoded as cv::Point2f-s) of each detected object
class Snapshot {
public:
	// Constructs a Snapshot with the given timestamp and an empty vector of positions
	Snapshot(float timestamp);
	// Constructs a snapshot from a given string description
	// a string description is FLOAT_VAL{#POINT}* where
	//   FLOAT_VAL = a string representing a float value
	//   POINT = FLOAT_VAL:FLOAT_VAL
	// for example: 0.3#1.5:1.5#3.0:2.0 is the string representation of a snapshot with timestamp 0.3 and a list
	// of two detected objects at coordinates (1.5, 1.5) and (3.0, 2.0)
	// WARNING/FIXME: Assumes the string is well formed
	Snapshot(std::string description);

	~Snapshot();

	// Adds an object position to the snapshot
	void AddObject(cv::Point2f point) { pts.push_back(point); }
	// Returns a string description of the Snapshot
	std::string str() const;

	// Returns the timestamp of the Snapshot
	float Time() const { return tstamp; }
	// Returns a const reference to the vector of positions
	const std::vector<cv::Point2f> & Data() const { return pts; }
	// Returns the number of objects contained by the Snapshot
	size_t size() const { return pts.size(); }

	// Random access to the vector of points
	cv::Point2f & operator[](size_t i) { return pts[i]; }
	// Comparison based on the timestamp
	bool operator <(const Snapshot &other) const { return tstamp < other.tstamp; }

private:
	float tstamp;
	std::vector<cv::Point2f> pts;
};

}

#endif
