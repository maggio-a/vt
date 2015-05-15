
#include "SynchronizedPriorityQueue.hpp"

extern SynchronizedPriorityQueue<Snapshot> snapshots;

void *Aggregator(void *arg) {
	//sleep(2); wait for some data to be available
	vector<rhs::MovingObject> objects;
	int objectCount = 0;

	float lastUpdate = 0.0f;

	Snapshot snap = snapshots.Pop();
	for (size_t i = 0; i < snap.size(); ++i) {
		stringstream ss;
		ss << "OBJ  " << (objectCount++);
		objects.push_back(MovingObject(ss.str(),snap[i]));
	}

	Mat measurement(2, 1, CV_32F);
	while (true) {
		snap = snapshots.Pop();
		float dt = snap.time() - lastUpdate;

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
						measurement.at<float>(0) = match.x;
						measurement.at<float>(1) = match.y;
						assert(used.insert(j).second == true);
						tracker.feedback(measurement);
					} else {
						cout << "FIXME object without detection" << endl;
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

		cout << "NEED TO SHOW SOMETHING HERE" << endl;
	}
}