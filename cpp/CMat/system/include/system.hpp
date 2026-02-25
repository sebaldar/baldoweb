#ifndef _system_
#define _system_

#include <iostream>
#include <map>

#ifndef _cmat_
   #include <cmat.hpp>
#endif

#ifndef _variable_
   #include <variabile.hpp>
#endif

#ifndef _funzione_
   #include <funzione.hpp>
#endif


#ifndef __xml__
	#include <xml.hpp>
#endif

#ifndef _nozzle_
	#include <nozzle.hpp>
#endif

#include <ext_funz.hpp>


class System
{

private:


	std::vector< Variabile *> v_var;
	std::vector< Funzione * > v_funz;

	class Equazione : public $EXT_FUNZ
	{

		class System * system ;

	public :

		Equazione ( class System * sys );
		~Equazione() ;

		REAL & callback ( const char * );
		void command ( const char * );

	};

	std::string nome;
	std::string descrizione;
	std::string document_root;
	

	std::map< std::string, class LINEA * > lines ;
	std::map< std::string, class COMPONENTE * > components ;

	NOZZLE * RegistraCompDaLinea ( LINEA * linea, const std::string &nome, const std::string &verso );
	bool RegistraVariabili ( COMPONENTE & comp, XMLElement * el ) ;

	bool RegistraTabelle ( XMLDocument & doc );
	bool processTab ( const std::string & id, XMLElement * tab );
	bool RegistraLinee ( XMLDocument &doc ) ;
	bool RegistraComponenti ( XMLDocument &doc ) ;
	bool processComponente ( const std::string &id, XMLElement * c ) ;
	bool Inizializzazione ( XMLDocument &doc );
	
	bool setEquazione ( COMPONENTE & c, const std::string & equazione );
	Variabile *  impostaVariabile ( COMPONENTE & comp, const std::string & id );
	bool initVariabile ( const std::string & riga, const std::string &def ) ;

protected :

	Math < Funzione, Variabile, double > m;

public:

	System ( );
	virtual ~System ( );

	std::string formatProgramm ( const std::string & prog );

	void init ( const std::string & src );
	
	std::string getXML () ;

	bool Execute ();
	std::string   plot ( int f, int v );

	double variable ( const std::string & nome_variabile ) const ;
	std::map< std::string, double > mapVariable ( ) const ;

};
#endif
