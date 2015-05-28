// =============================================================================
//
//  This file is part of the final project source code for the course "Ad hoc
//  and sensor networks" (Master's degree in Computer Science, University of
//  Pisa)
//
//  Copyright (C) 2015, Andrea Maggiordomo
//
// =============================================================================

#ifndef RHS_TIMER_HDR
#define RHS_TIMER_HDR

#include <time.h>

namespace rhs {

// class Timer
class Timer {
public:
	Timer() { this->Restart(); }
	~Timer() { }

	// Returns the time in seconds from the last Restart()
	float TimeElapsed() const {
		struct timespec now;
		::clock_gettime(CLOCK_MONOTONIC, &now);
		return float(((now.tv_sec*1000.0 + now.tv_nsec/1000000.0) - (tbase.tv_sec*1000.0 + tbase.tv_nsec/1000000.0)) / 1000.0);
	}

	void Restart() { ::clock_gettime(CLOCK_MONOTONIC, &tbase); }

private:
	struct timespec tbase;
};

} // rhs namespace

#endif
