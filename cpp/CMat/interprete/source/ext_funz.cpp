#include <sstream>
#include <stdlib.h>	// per exit()
#include <time.h>       /* time_t, struct tm, time, gmtime */

#include <ext_funz.hpp>

#ifndef _interprete_
	#include <Interprete.hpp>
#endif  // #ifndef _interprete_

unsigned short $EXT_FUNZ::id_programm = 0;

$EXT_FUNZ::$EXT_FUNZ ()  :
	$EXT_FUNZ ( new TINTERPRETE() ) {

    to_be_deleted = true;

}

$EXT_FUNZ::$EXT_FUNZ ( const std::string & source ) :
	$EXT_FUNZ ( new TINTERPRETE() ) {

    to_be_deleted = true;
	setSource( source );
}

$EXT_FUNZ::$EXT_FUNZ ( TINTERPRETE * _interprete )  :
	Funzione (), interprete( _interprete ), to_be_deleted ( false )  {

	interprete->SetExt$Funz (  this, & $EXT_FUNZ::callback ) ;
	interprete->SetCommandFunz (  this, & $EXT_FUNZ::command ) ;

	// genera un nome casuale
	 std::stringstream ss;
	 ss << "prj#" << id_programm;
	 id_programm++;

	 nome_prg = ss.str();


}

$EXT_FUNZ::~$EXT_FUNZ ()
{
	if ( to_be_deleted)
        delete interprete;
}

void $EXT_FUNZ::setSource ( const std::string & src )
{

    source = src;

    if ( interprete->existsProgramm(  ) )
        interprete->ClearProgramm(  ) ;

}


REAL $EXT_FUNZ::execute ( const std::string & the_source)
{

    setSource ( the_source );

	return execute () ;

}

REAL $EXT_FUNZ::execute ( ) const
{

    // esegue

    if ( !interprete->existsProgramm(  ) ) {

    	try {
            interprete->RegisterProgramm ( source.c_str() ) ;
        } catch ( const std::string & err ) {
            throw ( err );
        }

    }

	try {
		return interprete->ExecProgramm ( ) ;

	} catch ( const std::string & err ) {
        throw ( err );
	}


}

REAL $EXT_FUNZ::forceExecute ( ) const
{
    // forza la compilazione prima di eseguirlo

    if ( interprete->existsProgramm( ) )
        interprete->ClearProgramm(  ) ;

    return execute();

}


REAL $EXT_FUNZ::operator ()() const
 {
    return execute ();
 }

