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
#include <memory>

#include "thread.hpp"
#include "SynchronizedPriorityQueue.hpp"
#include "Msg.hpp"
#include "Socket.hpp"
#include "Snapshot.hpp"


using namespace rhs;
using namespace std;

extern shared_ptr< vector<socketHandle_t> > connections;

void *Aggregator(void *arg);

void *Receiver(void *arg) {

	SynchronizedPriorityQueue<Snapshot> queue;

	thread agg(Aggregator, &queue);

	//if streaming[i] == true then connections[i] is receiving data
	vector<bool> streaming(connections->size(), false);

	while (true) {
		bool allready = true;
		for (size_t i = 0; i < streaming.size(); i++) {
			socketHandle_t channel = (*connections)[i];
			if (!streaming[i]) {
				Message m = channel->Receive();
				if (m.type == Message::STREAM_START)
					streaming[i] = true;
				else //received wrong message, keep iterating and hope the next one is right (fixme)
					allready = false;
			}
		}
		if (allready) break;
	}

	while (true) {
		bool alldone = true;
		for (size_t i = 0; i < streaming.size(); i++) {
			if (streaming[i]) {
				alldone = false;
				socketHandle_t channel = (*connections)[i];
				try {
					Message m = channel->Receive(50); // wait for 50 msec
					if (m.type == Message::OBJECT_DATA) {
						//cout << i << ": " << m.payload.substr(0, 10) << endl;
						queue.Push(Snapshot(m.payload));
					} else if (m.type == Message::STREAM_STOP) { // this channel has stopped streaming
						streaming[i] = false; 
					}
				} catch (Socket::Timeout) {  } // skip
			}
		}
		if (alldone) break;
	}

	queue.Close();
	agg.join();

	cout << "Receiver stopped\n";

	return 0;
}
