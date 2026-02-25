#include <timer.hpp>

void Timer::start() {

    epoch = Clock::now();
    status = ON;

}

void Timer::stop() {

    status = OFF;

}

std::chrono::high_resolution_clock::duration Timer::time_elapsed() const {

    switch ( status ) {
	
		case ON :
		break;
		case OFF :
		break;
	
	}
    
    return Clock::now() - epoch;

}

double Timer::secs_elapsed() const {

    
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>( Clock::now() - epoch );
    return time_span.count();

}
