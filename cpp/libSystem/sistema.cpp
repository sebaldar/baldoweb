
#ifndef _sistema_
	#include <sistema.hpp>
#endif

#ifndef _system_
	#include <system.hpp>
#endif

#include <cgi_base.hpp>

Sistema::Sistema ( ) :
	system ( new System() )
{

	CGI cgi ;

	std::string documento = system->formatProgramm ( cgi.query_string() ) ;

	system->init ( documento );
	
	verbose ( false );

}

Sistema::Sistema ( const std::string & sistema ) :
	system ( new System() )
{

	std::string documento = system->formatProgramm ( sistema ) ;

	system->init ( documento );
	
	verbose ( false );

}

Sistema::~Sistema ( )
{
	delete system;
}

bool Sistema::esegui () const
{

	return system->Execute();

}

double Sistema::variabile ( const std::string & nome ) const
{
	
	return system->variable( nome );

}

std::map< std::string, double > Sistema::mapVariabili ( ) const 
{
	return system->mapVariable();
}

void Sistema::verbose ( bool show ) const
{
//	return system->verbose ( show );
}
