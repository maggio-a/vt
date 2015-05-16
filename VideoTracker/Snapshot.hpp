#ifndef RHS_SNAPSHOT_HDR
#define RHS_SNAPSHOT_HDR

#include <string>
#include <opencv2/core/core.hpp>

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
	cv::Point2f & operator[](size_t i) { return pts[i]; }
	bool operator <(const Snapshot &other) const { return tstamp < other.tstamp; }

private:
	float tstamp;
	std::vector<cv::Point2f> pts;
};

}

#endif