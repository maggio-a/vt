// =============================================================================
//
//  This file is part of the final project source code for the course "Ad hoc
//  and sensor networks" (Master's degree in Computer Science, University of
//  Pisa)
//
//  Copyright (C) 2015, Andrea Maggiordomo
//
// =============================================================================

#ifndef RHS_THREAD_HDR
#define RHS_THREAD_HDR

#include <pthread.h>

namespace rhs {

// class thread
// Just wraps the thread id and allows to join, eventually the code should be ported to use C++11 threads
// EXCEPTIONS: If any error occurs, the error code is thrown
class thread {
public:
	// Thread is spawned upon construction
	thread(void *(*fptr) (void *), void *arg) {
		int status = pthread_create(&tid, NULL, fptr, arg);
		if (status) 
			throw status;
	}

	~thread() {  }

	void join() {
		int status = pthread_join(tid, NULL);
		if (status)
			throw status;
	}

private:
	pthread_t tid;
};

} // rhs namespace

#endif
