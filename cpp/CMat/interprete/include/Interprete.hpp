#ifndef _interprete_
#define _interprete_

#include <iostream>

#include <ext_funz.hpp>

#ifndef _espressione_
	#include "Espressione.hpp"
#endif  // #ifndef _espressione_
//	definizione classe interprete

typedef TINTERPRETE Interprete;

class TINTERPRETE
{

	enum ComTag {

		COM_ESPRESSIONE,
		COM_VAI,
		COM_LABEL,
		COM_SE,
		COM_INIZIA,
		COM_FINE,
		COM_MENTRE,
		COM_NUMERO,
		COM_TABELLA,
		COM_MATRICE,
		COM_FUNZIONE,
		COM_CICLO_PER,
		COM_COMMENTO,
		COM_STAMPA,
		COM_STRINGA,
		COM_COMMAND

	} ;

	class TESPRESSIONE * Esp;

//	char *CommandTab[];

	ComTag GetTagCommand ( const char *com) const ;

// tabella hash per la memorizzazione delle etichette
	class HashClass *labels ;

    // gestione degli errori
	bool isError;
	void error( const char *msg) ;

public:

	TINTERPRETE();
	~TINTERPRETE();


private :

	class BASE_NODE * compiled ;
	bool isCompiled;

	class BASE_NODE *GetEndNode (BASE_NODE *node) ;

	// compila le righe del programma
	class BASE_NODE * CompileProgrammRows (BASE_NODE *start_node,
			BASE_NODE *end_node,
			char **row_array,
			int n_righe) ;

	BASE_NODE * CompileRow (BASE_NODE *current,
			BASE_NODE *end_node,
			char *riga) ;

	int RicavaRigheSottoProgramma (char ** prog, int riga_partenza, int righe_totali);

public :

	// registra e prepara un programma
	class BASE_NODE * CompileProgramm (const char *prog) ;
	void RegisterProgramm (const char *prog) ;

    bool existsProgramm ( ) const ;
	// processa il comando SE
	// esegue il programma di nome prog_name
	REAL ExecProgramm ( ) const ;
	void ClearProgramm( );


	// disalloca le variabili
	void ClearVariables () ;

	// FUNZIONI ESTERNE
	void InsertExtFunction  ( const char * nome_funzione, EXT_FUNZ funzione) ;

	void SetExt$Funz(  $EXT_FUNZ *n, $FUNZ f ) ;
	void SetCommandFunz(  $EXT_FUNZ *n, $COMMAND f ) ;

	$EXT_FUNZ * $node_funz;

// $COMMAND definito così -- typedef void ( $EXT_FUNZ::* $COMMAND) ( const char * );
	$COMMAND $command;

};

//	fine definizione classe interprete

// albero sintattico per la elaborazione di un programma interpretato

class BASE_NODE
{

protected:

	static bool finito ;

public:

	/*
	il nodo é giá stato visitato
	serve per disallocare il grafo sintattico del programma
	nella versione implementata visited é impostato a true
	dal distruttore. Logicamente imposta un valore in una zona di
	memoria disallocata, ma che rimane disponibile alla lettura
	Potrebbe dare dei problemi ed é quindi da modificare appena
	trovato un algoritmo migliore
	*/

	bool visited;

	BASE_NODE *next ;

	BASE_NODE ();

	virtual ~BASE_NODE ();

	virtual void eval () = 0;
	virtual const char * get_class () = 0 ;

} ;

class LABEL_NODE  : public BASE_NODE
{

public:

	LABEL_NODE ();
	~LABEL_NODE ();

	void eval () ;
	const char * get_class () {
		return "LABEL" ;
	}

} ;

class START_NODE : public BASE_NODE
{

public:

	START_NODE () ;
	~START_NODE ();

	void eval () ;
	const char * get_class () {
		return "START" ;
	}

} ;

class END_NODE : public BASE_NODE
{

public:

	END_NODE () ;
	~END_NODE ();

	void eval () ;

	const char * get_class () {
		return "END" ;
	}

} ;

class ESP_NODE : public BASE_NODE
{

	class TNODO *radice ;

public:

	ESP_NODE ( class TNODO *radice) ;
	~ESP_NODE ();

	void eval () ;
	const char * get_class () {
		return "ESP" ;
	}

} ;

class COMMAND_NODE : public BASE_NODE
{

protected:

	class TINTERPRETE * interpreter ;

	std::string command ;

public:

	COMMAND_NODE ( class TINTERPRETE * interpreter, const std::string & command ) ;
	~COMMAND_NODE ();

	void eval () ;
	const char * get_class () {
		return "COMMAND" ;
	}

} ;

class STAMPA_NODE : public BASE_NODE
{

	std::string to_print ;
	void stampa();

public:

    STAMPA_NODE ( char * str ) ;
    ~STAMPA_NODE ();

    void eval () ;


    const char * get_class () ;

} ;

class SE_NODE : public BASE_NODE
{

    TNODO *condizione ;

    BASE_NODE *cond_vero ;
    BASE_NODE *cond_falso ;

public:

    SE_NODE (TNODO *condizione) ;
    ~SE_NODE ();

    void eval () ;

    void SetCondVero (BASE_NODE *node) {
        cond_vero = node ;
    }

    void SetCondFalso (BASE_NODE *node) {
        cond_falso = node ;
    }

    const char * get_class () ;

} ;

class MENTRE_NODE : public BASE_NODE
{

	TNODO *condizione ;

    BASE_NODE *cond_vero ;

public:

    MENTRE_NODE (TNODO *condizione) ;
    ~MENTRE_NODE ();

    void eval () ;

    void SetCondVero (BASE_NODE *node) {
        cond_vero = node ;
    }

    const char * get_class () ;

} ;

class CICLO_PER_NODE : public BASE_NODE
{

	TNODO * inizializzazione ;
	TNODO * condizione ;
	TNODO * incremento ;

    BASE_NODE *cond_vero ;

public:

    CICLO_PER_NODE (TNODO * inizializzazione, TNODO * condizione, TNODO * incremento) ;
    ~CICLO_PER_NODE ();

    void eval () ;

    void SetCondVero (BASE_NODE *node) {
        cond_vero = node ;
    }

    const char * get_class () ;


} ;

#endif
