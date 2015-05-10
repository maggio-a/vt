#ifndef RHS_TIMER_HDR
#define RHS_TIMER_HDR

#include <time.h>

namespace rhs{

class Timer {
public:
	Timer();
	~Timer();

	float timeElapsed() const;
	void restart();

private:
	struct timespec tbase;
};

Timer::Timer() {
	this->restart();
}

Timer::~Timer() {

}

float Timer::TimeElapsed() const {
	struct timespec now;
	::clock_gettime(CLOCK_MONOTONIC, &now);
	return float(((now.tv_sec*1000.0 + now.tv_nsec/1000000.0) - (tbase.tv_sec*1000.0 + tbase.tv_nsec/1000000.0)) / 1000.0);
}

Timer::restart() {
	::clock_gettime(CLOCK_MONOTONIC, &base);
}

} // rhs namespace

#endif