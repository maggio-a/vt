#include <iostream>
#include <memory>

#include "thread.hpp"
#include "SynchronizedPriorityQueue.hpp"
#include "msg.hpp"
#include "Socket.hpp"
#include "Snapshot.hpp"


using namespace rhs;
using namespace std;

extern shared_ptr< vector< unique_ptr<Socket> > > connections;

void *Aggregator(void *arg);

void *Receiver(void *arg) {
	// activate remote sensors
	for (auto &channel : *connections) {
		channel->Send(rhs::Message(rhs::START_CAMERA, string("Hello!")));
	}

	SynchronizedPriorityQueue<Snapshot> queue;

	thread agg(Aggregator, &queue);

	//if streaming[i] == true then connections[i] is receiving data
	vector<bool> streaming(connections->size(), false);

	while (true) {
		bool allready = true;
		for (size_t i = 0; i < streaming.size(); i++) {
			unique_ptr<Socket> &channel = (*connections)[i];
			if (!streaming[i]) {
				Message hdr = channel->Receive();
				if (hdr.type == rhs::STREAM_START)
					streaming[i] = true;
				else //received wrong message
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
				unique_ptr<Socket> &channel = (*connections)[i];
				try {
					Message m = channel->Receive(50);
					if (m.type == rhs::OBJECT_DATA) {
						cout << i << ": " << m.payload.substr(0, 10) << endl;
						queue.Push(Snapshot(m.payload));
					} else if (m.type == rhs::STREAM_STOP) { // this channel has stopped streaming
						streaming[i] = false; 
					}
				} catch (Socket::Timeout) {  }
			}
		}
		if (alldone) break;
	}

	agg.join();
	return 0;
}