#include <iostream>
#include <map>

#include <system.hpp>


int main (int argc, char *argv[])
{

    try {

        Sistema sistema ;
        sistema.verbose( true );
        sistema.esegui ();

        std::map< std::string, double > vars = sistema.mapVariabili();

        for ( auto it = vars.begin(); it != vars.end(); it++ ) {
            std::cout << it->first << "=" << it->second <<std::endl;
        }
    } catch ( const std::string & e ) {
        std::cout << e << std::endl;
    }

	return ( 0 );


}
