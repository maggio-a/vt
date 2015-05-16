#include <iostream>
#include <memory>

#include "thread.hpp"
#include "SynchronizedPriorityQueue.hpp"
#include "msg.hpp"
#include "Socket.hpp"
#include "Snapshot.hpp"


using namespace rhs;
using namespace std;

extern auto_ptr<Socket> channel;

void *Aggregator(void *arg);

void *Receiver(void *arg) {
	SynchronizedPriorityQueue<Snapshot> queue;

	thread agg(Aggregator, &queue);

	// we should have a more robust protocol
	// wait for STREAM_BEGIN msg, aggregate data until STREAM_END

	while (true) {
		try {
			Message m = channel->Receive(50);
			if (m.type == rhs::OBJECT_DATA) {
				queue.Push(Snapshot(m.payload));
			}
		} catch (Socket::Timeout) {  }
	}

	agg.join();
	return 0;
}