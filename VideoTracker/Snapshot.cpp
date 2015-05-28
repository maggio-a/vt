// =============================================================================
//
//  This file is part of the final project source code for the course "Ad hoc
//  and sensor networks" (Master's degree in Computer Science, University of
//  Pisa)
//
//  Copyright (C) 2015, Andrea Maggiordomo
//
// =============================================================================

#include "Snapshot.hpp"

#include <string>
#include <vector>
#include <sstream>
#include <opencv2/core/core.hpp>

#include <iostream>

using namespace std;
using cv::Point2f;

static void split(const string &s, char delimiter, vector<string> &tokens_out) {
	tokens_out.clear();
	stringstream ss(s);
	string tk;
	while (getline(ss, tk, delimiter)) {
		tokens_out.push_back(tk);
	}
}

namespace rhs { 

Snapshot::Snapshot(float timestamp) : tstamp(timestamp), pts() {

}

//FIXME assumes the string is well formed
// format is timestamp#pt1#pt2#pt3...
// each point is written as x:y
Snapshot::Snapshot(string description) : tstamp(0.0f), pts() {
	vector<string> tokens;
	split(description, '#', tokens);
	// reads timestamp
	tstamp = float( atof(tokens[0].c_str()) );
	if (tokens.size() > 1) { // reads detected points
		for (size_t i = 1; i < tokens.size(); ++i) {
			vector<string> coords;
			split(tokens[i], ':', coords);
			pts.push_back(Point2f(stof(coords[0]),stof(coords[1])));
		}
	}
}

Snapshot::~Snapshot() {

}

// format is timestamp#pt1#pt2#pt3...
// each point is written as x:y
string Snapshot::str() const {
	stringstream ss;
	ss << tstamp;
	for (size_t i = 0; i < pts.size(); ++i) {
		ss << "#" << pts[i].x << ":" << pts[i].y;
	}
	return ss.str();
}

} // rhs namespace
