#include <iostream>
#include <fstream>

#ifndef _system_
	#include <system.hpp>
#endif


#include <cgi_base.hpp>

// per Trim
#ifndef until_h
	#include "Utils.h"
#endif


// Help message for clueless users
void help() {
  
  std::cout << "typical: [(-p|--programma) ####] "
       <<   "[(-o|--option options) ####]\n";
  std::cout << "  where:\n";
  std::cout << "  -p (--programma) is the programm to be execute\n";
  std::cout << "     system: resolv system of non linear equation.\n";
  std::cout << "     plot: plot graphic of singol equation.\n";
  std::cout << "  -o (--option) option to be given the programm\n";
}


int main (int argc, char *argv[])
{


	System system ;

	std::string prog = "";
	// A home-grown parsing algorithm starts here
	for( int i = 1; i < argc; ++i) {

		if (strcmp(argv[i], "-p") == 0 ||

			strcmp(argv[i], "--programm") == 0) {

			if ( i + 1 == argc) {
				// error messages intermingled with parsing logic
				std::cout << "Invalid " << argv[i];
				std::cout << " parameter: no programm specified\n";
				help();
				exit(1); // multiple exit points in parsing algorithm
			}

			prog = argv[++i];  // parsing action goes here

		}
		else if (strcmp(argv[i], "-o") == 0 ||

			strcmp(argv[i], "--option") == 0) {

			if (i+1 == argc) {
				std::cout << "Invalid " << argv[i];
				std::cout << " parameter: no option id  specified\n";
				help();
				exit(1);
			}
//      network = argv[++i];
		}
		else if (strcmp(argv[i], "--version") == 0) {
			std::cout << "Version 1.0\n";
			exit(0);
		}
	}

	if ( prog == "" ) {

		help();
		exit(1);
	}
	else if ( prog == "system" ) {

		try 	{

			CGI cgi ;
			std::string documento = system.formatProgramm ( cgi.query_string() ) ;

			system.init ( documento );
			system.Execute ();

			std::cout << system.getXML() << std::endl;

		}
		catch ( const std::string & e ) {
			std::cerr << "E' stato riscontrato un errore fatale durante l'esecuzione:" << std::endl;
			std::cerr << e << std::endl;
			exit(1);
		}

	}

	return (0);

}

