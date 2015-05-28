// =============================================================================
//
//  This file is part of the final project source code for the course "Ad hoc
//  and sensor networks" (Master's degree in Computer Science, University of
//  Pisa)
//
//  Copyright (C) 2015, Andrea Maggiordomo
//
// =============================================================================

#include <iostream>
#include <vector>
#include <sstream>
#include <set>
#include <deque>
#include <cstdlib>
#include <cmath>
#include <unistd.h>
#include <opencv2/opencv.hpp>

#include "SynchronizedPriorityQueue.hpp"
#include "Snapshot.hpp"
#include "MovingObject.hpp"
#include "hungarian.hpp"

using namespace std;
using namespace cv;
using namespace rhs;

extern float ROI[2];
extern int res[2];

static int zeroOffsetScale(float inMax, float outMax, float val) {
	float step = outMax / inMax;
	return int(val * step);

}

static Scalar colors[] = {
	Scalar(50 , 180,  96),
	Scalar(120,  48, 250),
	Scalar(150,  25,  60),
	Scalar( 75, 220, 200),
	Scalar(250,  50,  50),
	Scalar(190,  75, 130),
};

// Data associated with each tracked object
// encapsulates a Kalman filter (MovingObject) and a track (display position history)
// Position history in the ground plane reference is (at the moment) not recorded 
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

	void Draw(Mat &image, int roi_xmax, int roi_ymax) {
		int font = FONT_HERSHEY_PLAIN;
		double font_scale = 1.0;
		vector<Point2i> trace; // used to join previous positions with cv::polylines
		Size screen = image.size();
		for (auto &ipt : track) {
			if (ipt.x > 0 && ipt.x < roi_xmax && ipt.y > 0 && ipt.y < roi_ymax) {
				int px = zeroOffsetScale(roi_xmax, screen.width, ipt.x);
				int py = zeroOffsetScale(roi_ymax, screen.height, ipt.y);
				Point2i target = Point2i(px, screen.height-py); // flips the y coordinate

				circle(image, target, 5, color, 1, CV_AA);

				if (ipt == track.front()) {
					Point2f pt;
					switch (object.GetKfState()) {
					case MovingObject::INITIALIZED:
					case MovingObject::AFTER_UPDATE:
						pt = object.GetEstimatePost();
						break;
					case MovingObject::AFTER_PREDICT:
						pt = object.GetEstimatePre();
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
		bool operator()(const struct TrackingData &data) const { return (data.object.TimeFromLastMeasurement() > threshold); }
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
	//namedWindow(windowName);
	long distMax = long( std::sqrt(ROI[0]*ROI[0] + ROI[1]*ROI[1]) ); // used to pad the assignment matrix with fake columns
	while (true) {
		Mat image = Mat::zeros(res[1], res[0], CV_8UC3); // zeros(rows, columns, type): rows -> y, columns -> x

		try {
			snap = snapshots.Pop();
		} catch (SynchronizedPriorityQueue<Snapshot>::QueueClosed) {
			break;
		}
		float dt = snap.Time() - prevSnapTime;

		//cout << "dt:" << dt << "(" <<snap.time() << ", " << prevSnapTime << ")" << endl;
		if (dt <= 0) {
			cout << "skipping (dt = " << dt << ")\n";
			continue;
		}

		// Predict step
		vector<Point2f> predictions; // coupled with the objects array
		for (auto &td : data) {
			predictions.push_back(td.object.PredictPosition(dt));
		}

		// Compute matchings
		if (snap.size() > 0) { // if we detected objects
			set<size_t> used;
			if (predictions.size() > 0) { // if already tracking objects, compute a matching with detected objects
				vector<size_t> matching = ComputeMatching(predictions, snap.Data(), distMax);
				for (size_t i = 0; i < matching.size(); ++i) {
					MovingObject &tracker = data[i].object;
					size_t j = matching[i];
					if (j < snap.size()) { // snap[j] matches the prediction of data[i]
						// Update step
						Point2f match = snap[j];
						measurement.at<float>(0) = match.x;
						measurement.at<float>(1) = match.y;
						assert(used.insert(j).second == true); // snap[j] matched a prediction
						tracker.Feedback(measurement);
					} else {
						// The predicted position of data[i] did not match any detection
						// (j >= snap.size() implies that j was a fake column)
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
			switch (td.object.GetKfState()) {
			case MovingObject::INITIALIZED:
			case MovingObject::AFTER_UPDATE:
				pt = td.object.GetEstimatePost();
				break;
			case MovingObject::AFTER_PREDICT: // kalman filter was not updated
				pt = td.object.GetEstimatePre();
				break;
			}
			td.AddMarker(Point2i(pt.x, pt.y));
			td.Draw(image, ROI[0], ROI[1]);
		}

		// if trackers lost their object for more than a given threshold, remove them
		data.erase(remove_if(data.begin(), data.end(), TrackingData::Outdated(3.0f)), data.end());

		imshow(windowName, image);
		waitKey(5);
		
		int keyCode = waitKey(10);
		if ((keyCode == '\n' || (keyCode & 0xff) == '\n') || (keyCode == '\r' || (keyCode & 0xff) == '\r')) { // Enter key
			stringstream ss;
			ss << "Capture" << (c++) << ".png";
			imwrite(ss.str(), image);
		}

		prevSnapTime = snap.Time();
	}

	Mat image = Mat::zeros(res[1], res[0], CV_8UC3);
	imshow(windowName, image);
	//destroyWindow(windowName);
	waitKey(100);
	return 0;
}
