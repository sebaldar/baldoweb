#include <iostream>
#include <stdlib.h>


#include <ext_funz.hpp>

class Azione : public $EXT_FUNZ
{

	public :

	Azione ( const std::string & source ) : $EXT_FUNZ( source ) {
	}

	~Azione() {}

	void command ( const char * str) {
		std::cout << str << std::endl;
		}
	
	REAL & callback ( const char * val ) {

		static REAL null_rtr = 0;
		return null_rtr;

	}



};

int main (int argc, char *argv[])
{

	std::string source = "COMMAND pippo\nresult:=2+2/3";
	Azione azione ( source );
	REAL result = azione();
	std::cout << result << std::endl;

	return ( 0 );


}
