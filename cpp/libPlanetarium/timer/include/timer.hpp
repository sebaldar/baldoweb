#ifndef __timer__
#define __timer__

#include <chrono>


class Timer {

	
		enum STATUS {
			OFF,
			ON
		} status = OFF ;
    
    public:

        typedef std::chrono::high_resolution_clock Clock;

        void start() ;
        void stop() ;
        
        Clock::duration time_elapsed() const ;
		double secs_elapsed() const ;
        
        bool isRunning () {
			return ( status == ON ) ;
		}
		
		Timer () {
			start();
		}

		Timer ( const Timer & t) {
			epoch = t.epoch;
		}

		Timer &operator = ( const Timer & t) {
			epoch = t.epoch;
			return *this;
		}
    
    private:

         Clock::time_point epoch;

};


#endif


