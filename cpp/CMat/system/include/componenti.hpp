#ifndef _compon_
#define _compon_


class COMPONENTE  
{


protected:


public:


	std::string item;

	std::map< std::string, class NOZZLE * > nozzles ;
	std::map< std::string, class Variabile * > vars ;
	
	std::vector< class Funzione * > funzs ;

public:

	COMPONENTE  ( const std::string  & name ) :
		item(name) {}
	~COMPONENTE () {

		for ( auto it = nozzles.begin(); it != nozzles.end(); it++ ) {
			delete it->second;
		}

		for ( auto it = vars.begin(); it != vars.end(); it++ ) {
			delete it->second;
		}

		for ( auto it = funzs.begin(); it != funzs.end(); it++ ) {
			delete *it;
		}
		
		
	}


};



#endif
