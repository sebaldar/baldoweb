#ifndef _sistema_
#define _sistema_

#include <iostream>
#include <map>


class Sistema 
{


private:

	class System * system;

public:

	Sistema ( );	// riceve i dati dal pipe
	Sistema ( const std::string & sistema );
	virtual ~Sistema ( );
	
	bool esegui () const ;
	double variabile ( const std::string & nome ) const ;
	std::map< std::string, double > mapVariabili ( ) const ;
	void verbose ( bool show ) const ;

};
#endif
