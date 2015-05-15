#include <string>
#include <stringstream>
#include <opencv2/oopencv.hpp>

using namespace std;
using namespace cv;

void serialize(Socket s, vector<Point2f> objectData) {
	stringstream ss;
	// format is x1:y1#x2:y2#x3:y3...\0
	for (size_t i = 0; i < objectData.size(); ++i) {
		ss << objectData[i].x << ":" << objectData[i].y;
		if (i < objectData.size() - 1)
			ss << "#";
	}
	string data = ss.str();
	uint32_t nPoints = objectData.size();
	s.send(&nPoints, 4);
	s.send(data.c_str(), data.length()+1);
}

vector<Point2f> deserialize(Socket s) {
	uint32_t sz;
	s.recv(&sz, 4);
	char *buffer = new char[sz];
	s.recv(buffer, sz);
	buffer[sz-1] = '\0'; // just to be sure
	string data(buffer);
	delete[] buffer;

	vector<Point2f>
	vector<string> points;
	split(data, '#', points);
	for (size_t i = 0; i < points.size(); ++i) {
		vector<string> coords;
		split(points[i], ':', coords);
		out.push_back(Point2f(strtod(coords[0].c_str(),0), strtod(coords[1].c_str(),0)));
	}
	return out;
}

void split(const string &s, char delim, vector<string> &elements_out) {
	elements_out.clear();
	stringstream ss(s);
	string token;
	while (getline(ss, token, delim)) {
		elements_out.push_back(token);
	}
}