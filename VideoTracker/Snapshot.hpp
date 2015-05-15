#ifndef RHS_SNAPSHOT_HDR
#define RHS_SNAPSHOT_HDR

#include <string>
#include <sstream>
#include <opencv2/opencv.hpp>

namespace rhs {

class Snapshot {
public:
	Snapshot(float timestamp);
	Snapshot(std::string description);
	~Snapshot();

	void addObject(cv::Point2f point);
	std::string str();

	float time() { return tstamp; }
	const std::vector<cv::Point2f> data() { return pts; }
	size_t size() { return pts.size(); }
	const cv::Point2f &operator[](size_t i) { return pts[i]; }
	bool operator <(const Snapshot &s) { return tstamp < s.tstamp; }

private:
	float tstamp;
	std::vector<cv::Point2f> pts;
};

Snapshot::Snapshot(float timetamp) : tstamp(timestamp), pts() {

}

//FIXME assumes the string is well formed
Snapshot::Snapshot(std::string description) : tstamp(0.0f), pts() {
	std::vector<string> tokens;
	split(description, '#', tokens);
	tstamp = std::stof(tokens[0]);
	for (size_t i = 1; i < tokens.size(); ++i) {
		vector<string> coords;
		split(tokens[i], ':', coords);
		pts.push_back(Point2f(std::stof(coords[0]),std::stof(coords[1])));
	}
}

Snapshot::~Snapshot() {

}

void Snapshot::addObject(cv::Point2f point) {
	pts.push_back(point);
}

// format is timestamp#pt1#pt2#pt3...
// each point is written as x:y
std::string Snapshot::str() {
	std::stringstream ss;
	ss << tstamp << "#";
	for (size_t i = 0; i < pts.size(); ++i) {
		ss << "#" << pts[i].x << ":" << pts[i].y;
	}
	return ss.str();
}

}

#endif