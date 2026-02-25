#ifndef _funzione_
#define _funzione_

#include <iostream>
#include <sstream>

#ifndef _TYPEDEF_
using REAL = double ;
#endif	// if _TYPEDEF_


class Funzione {

public:

	std::string name;

	std::string source;
	std::string base;	// il sorgente base

protected :


public:

	Funzione () ;
	Funzione (Funzione &nd) ;

	virtual ~Funzione() {
		}

	Funzione & operator = ( Funzione & fz ) {

		source = fz.source;

		return *this;

	}

	// funzioni membro
	virtual REAL eval ()  = 0;
	double operator ()() ;

	std::string getSource() const ;
	void setSource( const std::string & src )  ;

};

#endif	// if _funzione_
