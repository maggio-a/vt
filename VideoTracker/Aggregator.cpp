#include <iostream>
#include <vector>
#include <sstream>
#include <set>
#include <unistd.h>
#include <opencv2/opencv.hpp>

#include "SynchronizedPriorityQueue.hpp"
#include "Snapshot.hpp"
#include "MovingObject.hpp"
#include "hungarian.hpp"

using namespace std;
using namespace cv;
using namespace rhs;

#define GROUND_WIDTH 650
#define GROUND_HEIGHT 650

void *Aggregator(void *arg) {
	SynchronizedPriorityQueue<Snapshot> &snapshots = *((SynchronizedPriorityQueue<Snapshot>*)arg);
	sleep(2); //wait for some data to be available
	vector<rhs::MovingObject> objects;
	int objectCount = 0;

	float prevSnapTime = 0.0f;

	Snapshot snap = snapshots.Pop();
	for (size_t i = 0; i < snap.size(); ++i) {
		stringstream ss;
		ss << "OBJ  " << (objectCount++);
		objects.push_back(MovingObject(ss.str(),snap[i]));
	}

	int font = FONT_HERSHEY_PLAIN;
    double font_scale = 1.0;
	Mat measurement(2, 1, CV_32F);
	Mat image = Mat::zeros(650, 650, CV_8UC3);
	while (true) {
		snap = snapshots.Pop();
		float dt = snap.time() - prevSnapTime;

		if (dt <= 0)
			continue;

		vector<Point2f> predictions; // coupled with the objects array
		for (size_t i = 0; i < objects.size(); ++i) {
			predictions.push_back(objects[i].predictPosition(dt));
		}

		// FIXME fix this, now only prints predictions if we have any detection i should probably print the statePost of the filters	
		if (snap.size() > 0) { // if we detected objects
			set<size_t> used;
			if (predictions.size() > 0) { // if already tracking objects, compute the matching 
				vector<size_t> matching = ComputeMatching(predictions, snap.data());
				for (size_t i = 0; i < matching.size(); ++i) {
					rhs::MovingObject &tracker = objects[i];
					size_t j = matching[i];
					if (j < snap.size()) {
						Point2f match = snap[j];
						Point2i intPt(match.x,match.y);
						intPt.y = GROUND_HEIGHT - intPt.y;
						if (intPt.x > 0 && intPt.x < GROUND_WIDTH && intPt.y > 0 && intPt.y < GROUND_HEIGHT) {
							circle(image, intPt, 5, Scalar(255,255,0), 1, CV_AA);
							putText(image, tracker.tag(), (intPt + Point2i(5,-5)), font, font_scale, Scalar(0,255,0));
						}

						measurement.at<float>(0) = match.x;
						measurement.at<float>(1) = match.y;
						assert(used.insert(j).second == true);
						tracker.feedback(measurement);
					} else {
						//cout << "FIXME object without detection" << endl;
					}
				}
			}
			// set up new trackers for unmatched detections (if any)
			for (size_t j = 0; j < snap.size(); ++j) {
				if (used.find(j) == used.end()) {
					stringstream ss;
					ss << "OBJ  " << (objectCount++);
					objects.push_back(rhs::MovingObject(ss.str(), snap[j]));
				}
			}
		}

		//cout << "NEED TO SHOW SOMETHING HERE" << endl;

		imshow("Plane view", image);
		waitKey(1);

		prevSnapTime = snap.time();
	}
}