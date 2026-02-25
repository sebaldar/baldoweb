#include <sstream>
#include "WSsrv.hpp"

int main ( int /* argc */, char ** /* argv */ )
{

	WSsrv solar;
			std::cout << "**" << std::endl;
	
	int socket = 1;
	std::stringstream system_data ;
	system_data <<  "<?xml version='1.0' encoding='UTF-8' ?>"
		<< "<body map_dir='/my_image' name='earth' map='$earth'>"
		<< "<body name='moon' map='$moon' />"
		<< "<body name='sky' map='$sky' />"
        << "</body>";

			std::cout << "***" << std::endl;

		try {
			
      std::stringstream query;
      query 
        << "{"
        << "\"user\" : \"user\","
        << "\"file\" : \"file\","
        << "\"document_root\" : \"document_root\","
        << "\"system_data\" : \"" << system_data.str() << "\"}";
			std::cout << "****" << std::endl;

      std::string xml = solar.registerClient ( socket, query.str() );
		
		}
		catch ( const std::string & ss ) {

			std::cout << ss << std::endl;
			std::cout << "**+++" << std::endl;

		}
	catch ( ... ) {

        std::cerr << "Catturata un'eccezione sconosciuta o generica." << std::endl;
	}


	return ( 0 );

}
