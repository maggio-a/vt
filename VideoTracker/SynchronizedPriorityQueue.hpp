#pragma once

#include <queue>
#include <pthread.h>

namespace rhs {

template<class T> class SynchronizedPriorityQueue {
public:
	SynchronizedPriorityQueue();
	~SynchronizedPriorityQueue();

	void Push(T item);
	T Pop();

private:
	pthread_mutex_t mtx;
	pthread_cond_t cond;

	std::priority_queue<T> pq;
};

template<class T> SynchronizedPriorityQueue<T>::SynchronizedPriorityQueue() : pq() {
	pthread_mutex_init(&mtx, 0);
	pthread_cond_init(&cond, 0);
}

template<class T> SynchronizedPriorityQueue<T>::~SynchronizedPriorityQueue() {
	pthread_mutex_destroy(&mtx);
	pthread_cond_destroy(&cond);
}

template<class T> void SynchronizedPriorityQueue<T>::Push(T item) {
	pthread_mutex_lock(&mtx);
	pq.push(item);
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mtx);
}

template<class T> T SynchronizedPriorityQueue<T>::Pop() {
	pthread_mutex_lock(&mtx);
	while (pq.empty()) {
		pthread_cond_wait(&cond, &mtx);
	}
	T item = pq.top();
	pq.pop();
	pthread_mutex_unlock(&mtx);
	return item;
}

} // rhs namespace