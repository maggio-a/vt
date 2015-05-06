#ifndef RHS_THREAD_HDR
#define RHS_THREAD_HDR

#include <pthread.h>

class thread {
public:
	thread(void *(*fptr) (void *), void *arg);
	~thread();

	void join();

private:
	pthread_t _tid;
};

thread::thread(void *(*fptr) (void *), void *arg) {
	int status = pthread_create(&_tid, NULL, fptr, arg);
	if (status) 
		throw status;
}

thread::~thread() {
	// nop
}

void thread::join() {
	int status = pthread_join(_tid, NULL);
	if (status)
		throw status;
}

#endif