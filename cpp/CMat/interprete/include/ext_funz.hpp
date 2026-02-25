#ifndef _ext_funz_
#define _ext_funz_

#include <iostream>


#ifndef _TYPEDEF_
   #include <typedef.hpp>
#endif

#ifndef _funzione_
   #include <funzione.hpp>
#endif


class $EXT_FUNZ : public Funzione {


		class TINTERPRETE * interprete;

        static unsigned short id_programm ;

//		std::string source;	// nella classe base
        std::string nome_prg;

        bool to_be_deleted = false ;     // indica se l'interprete deve essere cancellato

    protected :

	public:

		$EXT_FUNZ ( const std::string & source );
		$EXT_FUNZ ( class TINTERPRETE * );
		$EXT_FUNZ ();
		virtual ~$EXT_FUNZ ();

		// la funzione che implementa il callback alle variabili
		virtual REAL & callback ( const char * ) = 0 ;
		// la funzione che implementa COMMAND
		virtual void command ( const char * ) = 0 ;

        REAL operator ()() const ;

		REAL forceExecute ( ) const;
		REAL execute ( const std::string & ) ;
		virtual REAL execute ( ) const;

		// la funzione che esegue il sorgente
		REAL eval () {
			return execute ();
		}

		void setSource ( const std::string & source ) ;

};

typedef REAL & ( $EXT_FUNZ::* $FUNZ) ( const char * );
typedef void ( $EXT_FUNZ::* $COMMAND) ( const char * );


// puntatore a funzione che ritorna un float
// utilizzato per il nodo T_EXT_FUNZ
typedef REAL (*EXT_FUNZ) ( const char * );


#endif


