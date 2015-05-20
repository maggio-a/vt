#include <iostream>
#include <vector>
#include <sstream>
#include <set>
#include <deque>
#include <cstdlib>
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

static Scalar colors[] = {
	Scalar(50 , 180,  96),
	Scalar(120,  48, 250),
	Scalar(150,  25,  60),
	Scalar( 75, 220, 200),
	Scalar(250,  50,  50),
	Scalar(190,  75, 130),
};

struct TrackingData {
	MovingObject object;
	Scalar color;
	deque<Point2i> track;

	TrackingData(MovingObject o, Scalar c) : object(o), color(c), track() {  }

	void AddMarker(Point2i pt) {
		track.push_front(pt);
		if (track.size() > 200) {
			track.pop_back();
		}
	}

	void Draw(Mat &image, int roi_xmin, int roi_xmax, int roi_ymin, int roi_ymax) {
		int font = FONT_HERSHEY_PLAIN;
		double font_scale = 1.0;
		vector<Point2i> trace;
		for (auto &ipt : track) {
			if (ipt.x > roi_xmin && ipt.x < roi_xmax && ipt.y > 0 && ipt.y < roi_ymax) {
				Point2i target = Point2i(ipt.x,roi_ymax-ipt.y);
				circle(image, target, 5, color, 1, CV_AA);

				if (ipt == track.front()) {
					Point2f pt;
					switch (object.getObjectState()) {
					case MovingObject::INITIALIZED:
					case MovingObject::AFTER_UPDATE:
						pt = object.getEstimatePost();
						break;
					case MovingObject::AFTER_PREDICT:
						pt = object.getEstimatePre();
						break;
					}
					stringstream ss;
					ss << pt.x << ", " << pt.y;
					putText(image, ss.str(), (target + Point2i(5,-5)), font, font_scale, Scalar(0,255,0));
				}

				trace.push_back(target);
			}
		}
		Mat contour(trace);
		int npts = contour.rows;
		polylines(image, (const Point2i **)&contour.data, &npts, 1, false, Scalar(200,200,200), 1);//, CV_AA);
	} 

	// predicate for purging obsolete data
	struct Outdated {
		float threshold;
		Outdated(float th) : threshold(th) {  }
		bool operator()(const struct TrackingData &data) const { return (data.object.timeFromLastMeasurement() > threshold); }
	};
};

static const string windowName = "Plane view";

void *Aggregator(void *arg) {
	SynchronizedPriorityQueue<Snapshot> &snapshots = *((SynchronizedPriorityQueue<Snapshot>*)arg);
	sleep(3); //wait for some data to be available
	vector<TrackingData> data;
	int objectCount = 0;

	float prevSnapTime = 0.0f;

	Snapshot snap = snapshots.Pop();
	for (size_t i = 0; i < snap.size(); ++i) {
		stringstream ss;
		ss << "OBJ  " << (objectCount++);
		data.push_back( TrackingData(MovingObject(ss.str(),snap[i]), colors[rand()%6]) );
	}

	Mat measurement(2, 1, CV_32F);
	int c = 0;
	namedWindow(windowName);
	while (true) {
		Mat image = Mat::zeros(650, 650, CV_8UC3);

		try {
			snap = snapshots.Pop();
		} catch (SynchronizedPriorityQueue<Snapshot>::QueueClosed) {
			destroyWindow(windowName);
			waitKey(1000);
			return 0;
		}
		float dt = snap.time() - prevSnapTime;

		//cout << "dt:" << dt << "(" <<snap.time() << ", " << prevSnapTime << ")" << endl;
		if (dt <= 0) {
			cout << "skipping ********" << dt << endl;
		//	for (size_t i = 0; i < snap.size(); ++i) {
		//		cout << snap[i] << " " << endl;
		//	} cout << "*********\n";
		//	continue;
		}

		// Predict step
		vector<Point2f> predictions; // coupled with the objects array
		for (auto &td : data) {
			predictions.push_back(td.object.predictPosition(dt));
		}

		// Compute matchings
		if (snap.size() > 0) { // if we detected objects
			set<size_t> used;
			if (predictions.size() > 0) { // if already tracking objects, compute the matching 
				vector<size_t> matching = ComputeMatching(predictions, snap.data());
				for (size_t i = 0; i < matching.size(); ++i) {
					MovingObject &tracker = data[i].object;
					size_t j = matching[i];
					if (j < snap.size()) { // snap[j] matches the prediction of data[i]
						// Update step
						Point2f match = snap[j];
						measurement.at<float>(0) = match.x;
						measurement.at<float>(1) = match.y;
						assert(used.insert(j).second == true);
						tracker.feedback(measurement);
					} else {
						// The predicted position of data[i] did not match any detection
					}
				}
			}

			// set up new trackers for unmatched detections (if any)
			for (size_t j = 0; j < snap.size(); ++j) {
				if (used.find(j) == used.end()) {
					stringstream ss;
					ss << "OBJ  " << (objectCount++);
					data.push_back( TrackingData(MovingObject(ss.str(),snap[j]), colors[rand()%6]) );
				}
			}
		}

		for (auto &td : data) {
			Point2f pt;
			switch (td.object.getObjectState()) {
			case MovingObject::INITIALIZED:
			case MovingObject::AFTER_UPDATE:
				pt = td.object.getEstimatePost();
				break;
			case MovingObject::AFTER_PREDICT: // kalman filter was not updated
				pt = td.object.getEstimatePre();
				break;
			}
			td.AddMarker(Point2i(pt.x, pt.y));
			td.Draw(image, 0, GROUND_WIDTH, 0, GROUND_HEIGHT);
		}

		// if trackers lost their object for more than a given threshold, remove them
		data.erase(remove_if(data.begin(), data.end(), TrackingData::Outdated(3.0f)), data.end());

		imshow(windowName, image);
		
		int keyCode = waitKey(10);
		if ((keyCode == '\n' || (keyCode & 0xff) == '\n') || (keyCode == '\r' || (keyCode & 0xff) == '\r')) { // Enter key
			stringstream ss;
			ss << "Capture" << (c++) << ".png";
			imwrite(ss.str(), image);
		}

		prevSnapTime = snap.time();
	}

	return 0;
}