#pragma once

#include <queue>
#include <pthread.h>

namespace rhs {

// class SynchronizedPriorityQueue
// Provides synchronization in a producer(s)-consumer(s) setting. Allows a producer to close the queue
// in order to notify a consumer not to wait for further insertions
template<class T> class SynchronizedPriorityQueue {
public:
	// thrown when attempting to perform an operation on a closed queue
	struct QueueClosed {  };

	// Constructs an open empty queue
	SynchronizedPriorityQueue() : pq(), closed(false) {
		pthread_mutex_init(&mtx, 0);
		pthread_cond_init(&cond, 0);
	}

	~SynchronizedPriorityQueue() {
		pthread_mutex_destroy(&mtx);
		pthread_cond_destroy(&cond);
	}

	// Inserts an item in the queue. Throws SynchronizedPriorityQueue::QueueClosed if the queue is closed
	void Push(T item) {
		pthread_mutex_lock(&mtx);
		if (closed) {
			pthread_mutex_unlock(&mtx);
			throw QueueClosed();
		}
		pq.push(item);
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&mtx);
	}

	// Removes and returns the first element of the queue, blocking if the queue is empty.
	// Throws SynchronizedPriorityQueue::QueueClosed if the queue is empty and closed,a voiding to block indefinitely
	T Pop() {
		pthread_mutex_lock(&mtx);
		while (pq.empty() && !closed) {
			pthread_cond_wait(&cond, &mtx);
		}
		if (pq.empty() && closed) {
			pthread_mutex_unlock(&mtx);
			throw QueueClosed();
		} else {
			T item = pq.top();
			pq.pop();
			pthread_mutex_unlock(&mtx);
			return item;
		}
	}

	// Closes the queue, signaling any waiting consumer of the event
	void Close() {
		pthread_mutex_lock(&mtx);
		closed = true;
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&mtx);
	}

private:
	pthread_mutex_t mtx;
	pthread_cond_t cond;

	std::priority_queue<T> pq;
	bool closed;
};

} // rhs namespace