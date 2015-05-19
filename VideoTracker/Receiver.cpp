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

	// we should have a more robust protocol
	// wait for STREAM_BEGIN msg, aggregate data until STREAM_END

	while (true) {
		int i = 0;
		for (auto &channel : *connections) {
			i++;
			try {
				Message m = channel->Receive(50);
				if (m.type == rhs::OBJECT_DATA) {
					cout << i << ": " << m.payload.substr(0, 10) << endl;
					queue.Push(Snapshot(m.payload));
				}
			} catch (Socket::Timeout) {  }
		}
	}

	agg.join();
	return 0;
}