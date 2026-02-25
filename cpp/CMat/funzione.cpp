#include <sstream>

#ifndef _funzione_
   #include <funzione.hpp>
#endif


// implementazione della classe TFUNCTION
// costruttori
Funzione::Funzione () :
	source ()
{
}

Funzione::Funzione(Funzione &nd) :
	source( nd.source )
{
}

	// funzioni membro
double Funzione::operator ()() 
{
// ritorna la funzione della classe TNODE
	return eval() ;

}


std::string Funzione::getSource () const
{
	return source;
}

void Funzione::setSource ( const std::string & src ) 
{
	source = src;
	
}
