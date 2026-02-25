#include <sstream>
#include <iostream>
#include <fstream>
#include <string>

#include "../include/timer.hpp"

int main(void)
{

		Timer timer ;
		while ( true ) {
			
			// la scansione del web
			
			const auto elapsed = timer.time_elapsed();
			milliseconds msecs = duration_cast<milliseconds>(elapsed);

			if ( msecs > milliseconds( 1000 * 1 ) ) {	// ogni minuto

std::cout << "---" << std::endl;			
				
				timer.start();

			}
		
/*
*/
		}

	return 0;

}
