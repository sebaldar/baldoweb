#include <iostream>

#ifndef _system_
	#include <system.hpp>
#endif

#include <interfaccia.hpp>


std::string programm ( std::string &str )
{

	try 	{

		System system ;
		std::string documento = system.formatProgramm ( str ) ;

		system.init ( documento );
		system.Execute ();

		return system.getXML() ;

	}
	catch ( const std::string & e ) {
		std::cerr << "E' stato riscontrato un errore fatale durante l'esecuzione:" << std::endl;
		std::cerr << e << std::endl;
		exit(1);
	}


}

