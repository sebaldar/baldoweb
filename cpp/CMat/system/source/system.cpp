
#ifndef _system_
	#include <system.hpp>
#endif

#include <iostream>
#include <sstream>


// per Trim
#ifndef until_h
	#include "Utils.h"
#endif


#ifndef _TNODO_
	#include <ITRNode.hpp>
#endif


#ifndef _line_
	#include <Line.hpp>
#endif

#ifndef _nozzle_
	#include <nozzle.hpp>
#endif

#ifndef _compon_
	#include <componenti.hpp>
#endif

namespace std {
  template <typename _CharT, typename _Traits>
  inline basic_ostream<_CharT, _Traits> &
  tab(basic_ostream<_CharT, _Traits> &__os) {
    return __os.put(__os.widen('\t'));
  }
}

System::System ( )
{
		
}

System::~System ()
{

	for ( auto it = components.begin(); it != components.end(); it++ ) {
		delete it->second;
	}

	for ( auto it = lines.begin(); it != lines.end(); it++ ) {
		delete it->second;
	}

}



void System::init ( const std::string & src )
{

	XMLDocument doc ;

	try 	{
		doc.parse ( src ) ;
	}
	catch ( const std::string & e ) {

		std::ostringstream oss ( "" );
		std::string d = src;
		d = d.substr(10) + " ... ";

		oss << "Non è stato possibile eseguire il parsing corretto del documento " << d << " in quanto" << std::endl;
		oss << e << std::endl;
		throw ( oss.str() ) ;

	}

	XMLElement * root = doc.rootElement () ;

	nome = root->getAttribute( "nome");
	descrizione = root->getAttribute( "descrizione");
	document_root = root->getAttribute( "document-root").AsString;
	if ( document_root != "" && document_root.at( document_root.size() - 1 ) != '/' )
		document_root += "/";

	std::string str_err = root->getAttribute( "errore");
	if ( str_err != "" ) {

		double errore = root->getAttribute( "errore").AsFloat;
		m.setPrecisione ( errore, errore, errore ) ;

	}

	for ( XMLNode * n = root->firstChild(); n; n = n->nextSibling() ) {

		switch ( n->type() ) {
			case XMLNode::TEXT :
			case XMLNode::DOCUMENT :
			case XMLNode::SCRIPT :
			{
				std::ostringstream oss ( "" );
				oss << "Attenzione il testo \"" << n->value() << "\" verrà ignorato." ;
				m.MessageOut ( oss.str() ) ;
			}
			break;
			case XMLNode::ELEMENT :
			{
				std::string val = n->value();
				if (
					val != "iniz" &&
					val != "componente" &&
					val != "linea" &&
					val != "tabella" ) {

						std::ostringstream oss ( "" );
						oss << "Attenzione il tag [" << n->value() << "] non può essere utilizzato in questo contesto" ;
						oss << " e verrà quindi ignorato." ;
						m.MessageOut ( oss.str() ) ;

				}
			}
			break;

			default :
			break;
		}

	}

	try {

		RegistraTabelle ( doc ) ;

		RegistraLinee ( doc ) ;

        RegistraComponenti ( doc ) ;

        Inizializzazione ( doc );


		// genera il vettore delle variabili e quello delle funzioni
		v_var.clear();
		v_funz.clear();
		for ( auto it = components.begin(); it != components.end(); it++ )  {

			COMPONENTE * no = it->second;
			for ( auto it = no->funzs.begin(); it != no->funzs.end(); it++ ) {

				Funzione * equ = *it;
				v_funz.push_back( equ );

			}

			for ( auto it = no->vars.begin(); it != no->vars.end(); it++ ) {

				Variabile * var = it->second;
				if ( !var->fix )
					v_var.push_back( var );

			}

		}


		for ( auto it = lines.begin(); it != lines.end(); it++ ) {

			LINEA * no = it->second ;
			for ( auto it = no->funzs.begin(); it != no->funzs.end(); it++ ) {

				Funzione * equ = *it;
				v_funz.push_back( equ );

			}

			for ( auto it = no->vars.begin(); it != no->vars.end(); it++ ) {

				Variabile * var = it->second;
				if ( !var->fix )
					v_var.push_back( var );

			}

		}

//        checkLinea () ;	// verifica che i collegamenti siano congruenti

	} catch ( const std::string & e ) {

		throw ( e );

	}

}

bool System::Execute ()
{

	try {
		return m.execute ( v_var, v_funz );
	}
	catch ( const std::string & err ) {
		throw ( err );
	}

	return false;

}


std::string System::formatProgramm ( const std::string & src )
{

    std::string prog = utils::trim ( src ) ;

    const std::string p = "<pro";
    const std::string x = "<?xml";

    if ( prog.substr ( 0, p.size() ) != p &&
        prog.substr ( 0, x.size() ) != x )

		return
			"<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>"
			"<progetto document-root=\"\" errore=\"0.0000000001\">"
			"	<componente ident=\"C\">"
			"		<equazione>"
				+	prog +
			"		</equazione>"
			"	</componente>"
			"</progetto>";

    return prog ;

}


bool System::RegistraTabelle ( XMLDocument & doc )
{

	NODES t = doc.getElementsByTagName("tabella");
	for ( size_t j = 0; j < t.size(); j++) {

		std::string src = utils::trim(t[j]->getAttribute("src"));
		std::string id = t[j]->getAttribute("ident");

		if ( src == "" && t[j]->hasChilds() ) {
            processTab ( id, t[j] );
		}
		else if ( src != "" ) {

			if ( src.at(0) != '/' )
				src = document_root + src;
			else
				src = document_root + src.substr(1);

            std::string alias = t[j]->getAttribute("alias");
			std::fstream is( src.c_str(), std::fstream::in );

			if ( is.is_open() ) {

				char * buffer;
				std::string source;
				
				// get length of file:
				is.seekg (0, std::ios::end);
				int   length = is.tellg();
				is.seekg (0, std::ios::beg);

				// allocate memory:
				buffer = new char [length + 1];
				buffer[length] = '\0';

				// read data as a block:
				is.read (buffer,length);
				is.close();

				source = buffer ;

 				XMLDocument tab_doc ;
                try {
                    tab_doc.parse ( source ) ;
                } catch ( std::string & e) {
                   throw ( e );
				}

				XMLElement * d = tab_doc.rootElement();
				if ( d->value() != "tabelle" ) {
					std::ostringstream oss ;
					oss << "Il tag radice nelle tabelle importate deve essere \"tabelle\" e non " << d->value();
					throw( oss.str() );
				}

                bool id_trovato = true;
				NODES v1 = d->getElementsByTagName("tabella");
				for ( size_t i = 0; i < v1.size(); i++) {

					std::string this_ident = v1[i]->getAttribute( "ident" );

					if ( this_ident == id ) {

                        if ( alias != "" ) id = alias;
                        processTab ( id,  v1[i] );

                        id_trovato = true;

                        break;

					}

				}

				if ( !id_trovato ) {
					std::ostringstream oss ( "" );
					oss << "Identificativo " << id << " non trovato nella tabella " << src;
					throw( oss.str() );
				}

				delete[] buffer;

			}
			else {
      			std::ostringstream oss ;
				oss << "Errore nell'apertura dei dati del componente " << id <<  " file " << src << std::endl;
//				oss << std::strerror(errno) << std::endl;
				throw ( oss.str() );
			}

		}

	}


	return true ;


}

bool System::processTab ( const std::string & id, XMLElement * tab )
{

    /*
    è una matrice se il tag dati contiene l'attributo valore non vuoto
    è una curva se il tag dati non contiene l'attributo valore
    la tabella deve essere o curva o matrice
    */
    bool isMatrix = false;
    bool isCurve = false;


    // verifica la congruenza dei dati
    XMLNode * n  = tab->firstChild();
    while ( n ) {

		// lo salvo prima perchè potrebbe essere cancellato da
		XMLNode * s  = n->nextSibling();

		if  ( n->type() == XMLNode::COMMENT ) {
			n->parentNode()->removeChild( n );
        }
		else if ( n->type() == XMLNode::ELEMENT && ( n->value() == "dato" || n->value() == "dati" ) ) {

            std::string valore = ( (XMLElement * ) n )->getAttribute( "valore" );
            if ( valore == "" )
                isCurve = true;
            else
                isMatrix = true;

        }
        else {

            std::ostringstream oss ( "" );
			oss << "Nella tabella " << id << " i tag permessi sono  \"dato\" e \"dati\"." << tab->innerHTML();
			throw ( oss.str() ) ;

        }

        n = s;

	}

    if ( isCurve && isMatrix ) {
        std::ostringstream oss ( "" );
        oss << "Nella tabella " << id << " i dati devono contenere tutti l'attributo valore o nessuno deve contenere l'attributo valore.";
        m.MessageOut ( oss.str() ) ;
    }
    else if ( isCurve ) {

		TTAB id_node ( id.c_str() );
		id_node.MemorizzaTabella ( tab ) ;

	}
	else if ( isMatrix ) {

		TAB_MATRICE id_node ( id.c_str() );
		id_node.MemorizzaTabella ( tab ) ;

	}

	return true;

}


bool System::RegistraLinee ( XMLDocument &doc )
{

	NODES t = doc.getElementsByTagName("linea");
	for ( size_t j = 0; j < t.size(); j++) {

		std::string ident = t[j]->getAttribute("ident");

		auto it = lines.find(ident);
		LINEA *p = nullptr;
		if ( it != lines.end() ) {
			p = it->second;
		}
		else {	// la linea non è ancora stata registrata

			p = new LINEA ( ident );
			lines.insert ( std::pair< std::string, class LINEA * >(ident,p) );

		}

		/*
		 * controlla i collegamenti della linea
		 * inlet : componente ingresso linea
		 * outlet : componente uscita linea
		 *
		 *
		*/
		std::string inlet = t[j]->getAttribute("inlet");
		std::string outlet = t[j]->getAttribute("outlet");

		if ( inlet != "" ) {
			NOZZLE * n = RegistraCompDaLinea ( p, inlet, "inlet" );
			p->in = n ;
		}
		if ( outlet != "" ) {
			NOZZLE * n = RegistraCompDaLinea ( p, outlet, "outlet"  );
			p->out = n ;
		}

		// definizione delle variabili interne alla linea
		RegistraVariabili ( *p, t[j] );

	}

	return true;

}


NOZZLE * System::RegistraCompDaLinea ( LINEA * linea, const std::string &nome, const std::string &verso )
{

    if ( nome == "" ) return nullptr;

	// la sintassi è COMPONENTE.NOZZLE
	std::string comp_name = "";
	std::string nozzle_name = "";

	size_t p = nome.find(".");
	if ( p != std::string::npos ) {
		comp_name = nome.substr( 0, p );
		nozzle_name = nome.substr( p+1 );
	}
	else {
		std::ostringstream oss ;
		oss << "La sintassi per il componente da collegare " << nome <<  " è COMPONENTE.NOZZLE" << std::endl;
		throw ( oss.str() );
	}

	// verifica se il nodo esiste, altrimenti lo crea
	auto it = components.find(comp_name);
	COMPONENTE *c = nullptr;
	if ( it != components.end() ) {
		c = it->second;
	}
	else {
		c = new COMPONENTE ( comp_name );
		components.insert ( std::pair< std::string, class COMPONENTE * >(comp_name,c) );
	}

	// verifica se il nozzle è registrato nel componente
	auto it_n = c->nozzles.find(nozzle_name);
	NOZZLE *n = nullptr;
	if ( it_n != c->nozzles.end() ) {

		n = it_n->second;
		// imposta la linea

		if ( n->linea == nullptr ) {
			n->linea = linea ;
		}
		else if ( n->linea != linea ) {

			std::ostringstream oss ;
			oss << "Nel nodo  " << comp_name ;
			oss << " il bocchello  " << nozzle_name ;
			oss << " è collegato a più linee"  ;

			throw( oss.str() );

		}


	}
	else {

		n = new NOZZLE ( c, nozzle_name );
		c->nozzles.insert ( std::pair< std::string, class NOZZLE * >(nozzle_name, n) );

	}


	if ( verso == "inlet" )
		linea->in = n;
	else
		linea->out = n;

	return n;

}


bool System::RegistraVariabili ( COMPONENTE & comp, XMLElement * el )
{
	// definizione delle variabili interne
	NODES v = el->getElementsByTagName("variabile");
	for ( size_t j = 0; j < v.size(); j++) {

		std::string id = v[j]->getAttribute( "ident" );
		double val = v[j]->getAttribute( "valore" ).AsFloat;
		std::string fix = std::string(v[j]->getAttribute( "fix" )) != "" ?
			v[j]->getAttribute( "fix" ) :
			v[j]->getAttribute( "const" ) ;

		auto it = comp.vars.find( id );
		Variabile *var = nullptr;
		if ( it != comp.vars.end() ) {
			var = it->second;
		}
		else {
			var = new Variabile ( id );
			comp.vars[ id ] = var;
//			comp.vars.insert ( std::pair< std::string, Variabile * > ( id, var ) );
		}


		var->val=val;
		var->name=id;
		if ( fix == "const" || fix == "fix" )
			var->fix = true ;
		else
			var->fix = false ;

		if ( std::string(v[j]->getAttribute( "min" )) != "" )
			var->min = v[j]->getAttribute( "min" ).AsFloat ;
		if ( std::string(v[j]->getAttribute( "max" )) != "" )
			var->max = v[j]->getAttribute( "max" ).AsFloat ;

	}

    return true;

}


bool System::RegistraComponenti ( XMLDocument &doc )
{

	XMLElement * root = doc.rootElement () ;


	NODES c = root->getElementsByTagName("componente");
	for ( size_t i = 0; i < c.size(); i++) {

		std::string id = c[i]->getAttribute( "ident" );
		std::string src = utils::trim(c[i]->getAttribute("src"));

		if ( src == "" && c[i]->hasChilds() ) {

			try 	{

				processComponente ( id, c[i] );

			}
			catch ( const std::string & e ) {
				
				throw( e );
			
			}

		}
		else if ( src != "" ) {

			if ( src.at(0) != '/' )
				src = document_root + src;
			else
				src = document_root + src.substr(1);

            std::string alias = c[i]->getAttribute("alias");
			std::string source = "";
			char * buffer;

			std::ifstream is;
			is.open ( src.c_str(), std::ios::binary );

			if ( is.good() ) {

				// get length of file:
				is.seekg (0, std::ios::end);
				int   length = is.tellg();
				is.seekg (0, std::ios::beg);

				// allocate memory:
				buffer = new char [length+1];
				buffer[length]='\0';

				// read data as a block:
				is.read (buffer,length);
				is.close();

				source = buffer ;

				XMLDocument tab_doc ;
				tab_doc.parse ( source ) ;

				XMLElement * d = tab_doc.rootElement();
				if ( d->value() != "componenti" ) {
					std::ostringstream oss ( "" );
					oss << "Il tag radice nei componenti importati deve essere \"componenti\" e non " << d->value();
					throw( oss.str() );
				}

                bool id_trovato = true;
				NODES v1 = d->getElementsByTagName("componente");
				for ( size_t i = 0; i < v1.size(); i++) {

					std::string this_ident = v1[i]->getAttribute( "ident" );

					if ( this_ident == id ) {

                        if ( alias != "" ) id = alias;

						try 	{

							processComponente ( id, c[i] );

						}
						catch ( const std::string & e ) {
							
							throw( e );
						
						}

                        id_trovato = true;

                        break;

					}

				}

				if ( !id_trovato ) {
					std::ostringstream oss ( "" );
					oss << "Identificativo " << id << " non trovato nel file esterno " << src;
					throw( oss.str() );
				}

				delete[] buffer;

			}
			else {
				std::ostringstream oss ;
				oss << "Errore nell'apertura dei dati del componente " << id <<  " file " << src << std::endl;
				throw ( oss.str() );
			}

		}

	}

	return true;

}


bool System::processComponente ( const std::string &id, XMLElement * c )
{


		auto it = components.find( id );
		COMPONENTE *c1 = nullptr;
		if ( it != components.end() ) {
			c1 = it->second;
		}
		else { // se non esiste già lo crea
			c1 = new COMPONENTE ( id );
			components[ id ] = c1;
		}


		// i nozzle (cioè i collegamenti con le linee)
		// li fa prima delle equazioni in modo che i bocchelli siano già definiti
		NODES n = c->getElementsByTagName("nozzle");
		for ( size_t j = 0; j < n.size(); j++) {

			std::string id = utils::trim(n[j]->getAttribute( "ident" ));

			// verifica se il nozzle è registrato nel componente
			auto it_n = c1->nozzles.find(id);
			NOZZLE *n = nullptr;
			if ( it_n != c1->nozzles.end() ) {

				n = it_n->second;

			}
			else {

				n = new NOZZLE ( c1, id );
				c1->nozzles.insert ( std::pair< std::string, class NOZZLE * >(id, n) );

			}


		}

		NODES e = c->getElementsByTagName("equazione");
		for ( size_t j = 0; j < e.size(); j++) {

			for ( XMLNode *n = e[j]->firstChild(); n; n=n->nextSibling() ) {

				switch ( n->type () ) {

					case XMLNode::ELEMENT :
					{

						std::string val = n->value();
						if (

							val != "iniz" &&
							val != "tabella" ) {

							std::ostringstream oss ( "" );
							oss << "Attenzione il tag [" << n->value() << "] non può essere utilizzato " ;
							oss << "come discendente di [" << n->parentNode()->value()  << "]";
							throw( oss.str() ) ;

						}

					}
						break;

					case XMLNode::TEXT : {

						std::vector< std::string > equazioni;
						utils::split( n->value(), "\n", equazioni );

						for ( size_t i = 0; i < equazioni.size(); i++ ) {

							std::string eq = utils::trim ( equazioni[i] );

							if ( eq != "" && eq[0] !=  '#' ) {

								try {
									setEquazione( *c1, eq );
								}
								catch ( const std::string & e ) {
									std::ostringstream oss ;
									oss << "Non è stata possibile la valutazione della equazione:" << eq << std::endl;
									oss << e << std::endl;
									throw(oss.str());
								}

							}

						}
					}

						break;

					case XMLNode::COMMENT :

						break;

					default : {

						std::ostringstream oss ( "" );
						oss << "Attenzione il tag [" << n->value() << "] non può essere utilizzato in questo contesto" ;
						throw( oss.str() ) ;

					}

				}

			}

		}

		// verifica se a tutti i bocchelli è associata una linea
		for ( auto it = c1->nozzles.begin(); it != c1->nozzles.end(); it++ ) {
			NOZZLE * n = it->second;
			LINEA * p = n->linea;
			if ( p ) {

				auto it = lines.find( p->item );
				if ( it != lines.end() )
					lines[ p->item ] = p ;

			}

		}


		// definizione delle variabili interne al componente
		RegistraVariabili ( *c1, c );

		return true;

}


bool System::setEquazione ( COMPONENTE & comp, const std::string & equazione )
{
	/*
	analizza il sorgente cercando le variabili e registrandole, inoltre
	antepone a ciascuna variabile il nome del nodo nella forma node.variabile
	infine registra l'equazione nel nodo
	*/

	std::string prg;

	std::string last_alfa;
	CH last_tipo = CH::SPACE;
	CH next_tipo = CH::SPACE;
	int parentesi = 0;

	const char *it = equazione.c_str();
	while ( *it  ) {

		CH tipo ;
		switch ( *it ) {

		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		case '.': {

			std::string num;
			while (  *it && (isdigit(*it) || *it=='.') ) {
				num += *it;
				it++;
			}
			prg += num;

			// verifica che contenga un solo punto o nessuno
			size_t p = num.find(".");
			if ( p != std::string::npos ) {
				p = num.substr(p+1).find(".");
				if ( p != std::string::npos ) {
					std::ostringstream oss ( "" );
					oss << std::tab << "Nella equazione " << equazione << " nodo " << comp.item << " il numero  " << num << " non ha un formato corretto.";
					throw( oss.str() );
				}
			}

			tipo = CH::NUM;

			/*
			numero seguito da variabile implica l'operatore
			di moltiplicazione
			*/
			if ( *it &&  (*it == '(' || isalpha(*it)) )
				prg += '*';
		}

			break;

		case ' ':

			while (  *it && isspace(*++it) ) ;

			tipo = CH::SPACE;

			break;

		case '+': case '-':
		case '*': case '/': case '^' : {

			prg += *it;
			while (  *it && isspace(*++it) ) ;
			tipo = CH::OPERATORE;

		}
			break;
		case '&':  case ':':
		case '>':  case '<':  case '!':
		{

			std::ostringstream oss ( "" );
			oss  << std::tab << "Nella equazione " << equazione << " nodo " << comp.item << " il carattere " << *it << " non è ammesso.";
			throw( oss.str() );

		}
			break;
		case '=':
		{
			prg += *it;
			while (  *it && isspace(*++it) ) ;
			tipo = CH::NONE;
		}
			break;
		case '(': case ')': {

			if ( *it == '(' ) parentesi++;
			if ( *it == ')' ) parentesi--;

			if ( parentesi < 0 ) {
				std::ostringstream oss ( "" );
				oss  << std::tab << "Nella equazione " << equazione << " nodo " << std::endl;
				oss  << std::tab << "le parentesi non sono bilanciate";
				throw( oss.str() );
			}

			prg += *it;
			char op = *it;
			while (  *it && isspace(*++it) ) ;
			tipo = CH::BRAKET;

			/*
			parentesi chiusa seguita da parentesi aperta o da variabile implica l'operatore
			di moltiplicazione
			*/
			if ( *it && op == ')' &&  (*it == '(' || isalpha(*it)) )
				prg += '*';

		}
			break;

		case ',':
			prg += *it;
			it++;
			tipo = CH::COMMA;
			break;

		case 'A':

			if (*(it + 1) == 'N' && *(it + 2) == 'D') {
				tipo =  CH::OPERATORE ;
				for (unsigned int i=0; i<3; i++,it++)
					prg += *it;
			}
			else {
				tipo = CH::ALFA ;
			}

			break ;

		case 'N' :
			if (*(it + 1) == 'O' && *(it + 2) == 'T') {
				tipo =  CH::OPERATORE ;
				for (unsigned int i=0; i<3; i++,it++)
					prg += *it;
			}
			else {
				tipo = CH::ALFA ;
			}

			break ;

		case 'O':
			if (*(it + 1) == 'R') {
				tipo =  CH::OPERATORE ;
				for (unsigned int i=0; i<2; i++,it++)
					prg += *it;
			}
			else {
				tipo = CH::ALFA ;
			}

			break ;

		default:
			tipo = CH::ALFA;
		}

		if ( tipo == CH::ALFA ) {

			if ( next_tipo == CH::OPERATORE ) {
				std::ostringstream oss ( "" );
				oss  << std::tab << "Nella equazione " << equazione << " nodo " << comp.item << " dopo il simbolo " << last_alfa << " era atteso un operatore o un fine riga.";
				throw( oss.str() );
			}

			const char * s1=it;

			while ( *it  && (isalpha( *it ) || isdigit( *it ) || *it == '.' )) it++;

			const char * s2=it-1;

			while (  *it && isspace( *it ) ) it++;

			if ( *it=='\0' || *it != '('  ) {	// non è funzione o tabella

				std::string id = "";
				for (; s1<=s2;s1++) id+=*s1;

				impostaVariabile ( comp, id );

				prg += comp.item + std::string(".") + id;
				last_alfa = id;

			}
			else {

				std::string id = "";
				for (; s1<=s2;s1++) id+=*s1;

				prg += id;
			}

			next_tipo = CH::OPERATORE;

		}
/*
		else if ( tipo == CH::NUM ) {

			if ( next_tipo == CH::OPERATORE ) {
				std::ostringstream oss ( "" );
				oss  << std::tab << "Nella equazione " << equazione << " nodo " << comp.item << " dopo il simbolo " << last_alfa << " era atteso un operatore o la fine della riga.";
				throw( oss.str() );
			}

			next_tipo = CH::OPERATORE;

		}
*/
		else if ( tipo == CH::OPERATORE ) {

			if ( last_tipo == CH::OPERATORE ) {
				std::ostringstream oss ( "" );
				oss  << std::tab << "Nella equazione " << equazione << " nodo " << comp.item << " dopo il simbolo " << last_alfa << " era attesa una variabile.";
				throw( oss.str() );
			}

			next_tipo = CH::ALFA;
		}
		else {
			next_tipo = CH::NONE;
		}

		last_tipo = tipo;

	}

	if ( prg == "" )
		return false;
	else {

		size_t p = prg.find( _RISULTATO );
		if ( p == std::string::npos ) {

			size_t p = prg.find( "=" );
			if ( p != std::string::npos ) {

				size_t p1 = p;
				while ( prg[p--] == ' ' );
				if ( prg[p] == '+' || prg[p] == '-' || prg[p] == '*'
						|| prg[p] == '/' || prg[p] == '^' ) {
					std::ostringstream oss ( "" );
					oss  << std::tab << "Prima del segno = non devono esserci operatori";
					throw( oss.str() );
				}

				p = p1;
				prg = "" + prg.substr(0,p) + "-(" + prg.substr(p+1) + ")";
				p = prg.substr(p+1).find( "=" );
				if ( p != std::string::npos ) {
					std::ostringstream oss ( "" );
					oss  << std::tab << "Nella equazione " << equazione << " nodo " << comp.item << " risulta più di un segno di uguaglianza.";
					throw( oss.str() );
				}

				// verifica che prima del segno uguale non ci siano operatori +,-,*,/,^
			}
			else {
				std::ostringstream oss ( "" );
				oss  << std::tab << "Nella equazione " << equazione << " nodo " << comp.item <<" manca il segno di uguaglianza.";
				throw( oss.str() );
			}

            std::string ris = std::string(_RISULTATO) + ":=";
			prg = ris + prg;

		}

		if ( parentesi != 0 ) {

			std::ostringstream oss ( "" );
			oss  << std::tab << "Nella equazione " << equazione << " nodo " << comp.item << std::endl;
			oss  << std::tab << "le parentesi aperte non sono uguali alle parentesi chiuse" << std::endl;
			throw( oss.str() );

		}



		Equazione * azione = new Equazione ( this );
		azione->source = prg;
		azione->base = equazione;

		size_t n = comp.funzs.size();

		std::stringstream nome_funzione ;
		nome_funzione << "f" << n;
		azione->name = nome_funzione.str() ;

		comp.funzs.push_back(azione);


		return true ;

	}

}


Variabile *  System::impostaVariabile ( COMPONENTE & comp, const std::string & id )
{

	Variabile * v ;
	// verifica se id contiene un punto
	// in questo caso si tratta di un bocchello
	size_t p = id.find(".");
	if ( p != std::string::npos ) {

		std::string noz_name = id.substr( 0, p );
		std::string var = id.substr( p + 1 );

		NOZZLE *nozzle = nullptr;
		auto it = comp.nozzles.find( noz_name );
		if ( it != comp.nozzles.end() ) {
			nozzle = it->second;
		}
		else {
			nozzle = new NOZZLE( &comp, noz_name );
			comp.nozzles[ noz_name ] = nozzle;
		}


		LINEA * linea = nozzle->linea;

		if ( linea ) {

			if ( linea->in != nozzle && linea->out != nozzle ) {

				std::string in, out;
				if ( linea->in )
					in = "bocchello " + linea->in->item;
				else
					in = " nessun bocchello " ;
				if ( linea->out )
					out = "bocchello " + linea->out->item;
				else
					out = " nessun bocchello " ;

				std::ostringstream oss ( "" );
				oss << std::tab << "Nel nodo " << comp.item << " il bocchello " << noz_name << std::endl;
				oss << std::tab << "E' collegato alla linea " << linea->item << " che è collegata " << std::endl;
				oss << std::tab << "Inlet " << in << std::endl;
				oss << std::tab << "Outlet " << out << std::endl;
				throw( oss.str() );

			}

			// verifica se esiste già la variabile, altrimenti la registra
			auto it = linea->vars.find( var );
			if ( it != linea->vars.end() ) {
				v = it->second;
			}
			else {
				v = new Variabile ( var );
				linea->vars[ var ] = v;
			}

		}
		else {

			// non c'è nessuna linea associata al bocchello
			// allora crea una linea nuova

			std::string ident = comp.item + "." + noz_name;
			linea = new LINEA ( ident );
			nozzle->linea = linea ;

			v = new Variabile ( var );
			linea->vars[ var ] = v;
/*
			ostringstream oss ( "" );
			oss << "Nel nodo " << Item() << " il bocchello " << noz_name << std::endl;
			oss << "Non è collegato a nessuna linea" << std::endl;
			throw( oss.str() );
*/

		}

	}
	else {

		// se non è un bocchello è una variabile interna che va registrata
		v = new Variabile ( id );
		comp.vars[ id ] = v;

	}

	return v;

}

bool System::Inizializzazione ( XMLDocument &doc )
{

	XMLElement * root = doc.rootElement () ;

	NODES c = root->getElementsByTagName("iniz");
	for ( size_t i = 0; i < c.size(); i++) {

		for ( XMLNode *n = c[i]->firstChild(); n; n=n->nextSibling() ) {

			switch ( n->type () ) {

				case XMLNode::TEXT : {

					std::vector< std::string > variabili;
					utils::split( n->value(), "\n", variabili );

					for ( size_t j = 0; j < variabili.size(); j++ ) {

						std::string riga = utils::trim( variabili[j] );

						if ( riga != "" ) {

							// cerca se il tag inizializzazione è interno
							// a un tag componente o linea
							std::string def = "";
							XMLNode * p = n->parentNode();

							for ( ; p; p = p->parentNode() ) {
								if ( p->value() == "componente" || p->value() == "linea" ) {
									def = ( ( XMLElement * ) p)->getAttribute( "ident" );
									break;
								}
							}

							try {
								initVariabile ( riga, def );
							}
							catch ( const std::string & e ) {
								std::ostringstream oss ;
								oss << "Inizializzazione : " << e << std::endl;
								throw ( oss.str() );
							}

						}

					}


				}

					break;

				case XMLNode::COMMENT :

						break;

				default : {

					std::ostringstream oss ( "" );
					oss << "Attenzione il tag [" << n->value() << "] non può essere utilizzato " ;
					oss << "come discendente di [" << n->parentNode()->value()  << "]";
					throw( oss.str() ) ;

				}

			}

		}

	}

    return true;
}


bool System::initVariabile ( const std::string & riga, const std::string &def )
{

	// sintassi
	// nome_variabile=<valore>,nome_variabile=<valore>, ....
	// divide la stringa usando come separatore la virgola
	std::vector < std::string > v;

	size_t iniz = 0;
	for ( size_t i = 0; i < riga.size(); i++ ) {

		if ( riga.substr(i, 5) == "range" ) {
			// cerca le parentesi
			// salta i blank
			i += 5;
			while ( isblank(riga[i])  ) i++;
			if ( riga[i] != '(' ) {
				throw ( std::string("Attesa una parentesi aperta dopo range") );
			}
			i++;
			while ( riga[i] && riga[i] != ')' ) i++;
			if ( riga[i] != ')' ) {
				throw ( std::string("Attesa una parentesi chiusa nella direttiva range") );
			}
			i++;
		}
		if ( riga[i] == ',' ) {
            v.push_back( riga.substr( iniz, i - iniz ) ) ;
            iniz = i + 1;
		}

	}
    v.push_back( riga.substr( iniz ) ) ;

    // processa la inizializzazione delle variabili
	for ( size_t i = 0; i < v.size(); i++) {

		std::string va = utils::trim( v[i] );
		if ( va== "" ) continue;

		// lo divide in variabile e valore
		std::vector <std::string> v1;
		utils::split ( v[i], "=", v1 );
		if ( v1.size() != 2 ) {
			std::ostringstream oss ( "" );
			oss << "Nella assegnazione " << v[i] << " la sintassi deve essere variabile=valore.";
			throw ( oss.str() ) ;
		}

		std::string var = utils::trim(v1[0]);
		// la parte destra è costituita dal valore, la parola chiave const e la parola chiave range
		std::string right = utils::trim(v1[1]);


		// cerca const e range
		size_t p_const = right.find("const");
		size_t p_range = right.find("range");

		size_t p;
		if ( p_const < p_range )
			p = p_const;
		else
			p = p_range;

		std::string value ;
		std::string fix = "";
		std::string range = "";
		if ( p != std::string::npos ) {

			value = right.substr( 0, p);
			right = utils::trim(right.substr( p ));

			// in quello che rimane cerca const e range
			for ( size_t j=0; j<right.size(); j++) {

				if ( right.substr(j, 5) == "range" ) {
					// cerca le parentesi
					// salta i blank
					size_t iniz = j;
					j += 5;
					while ( isspace(right[j])  ) i++;
					if ( right[j] != '(' ) {
						throw ( std::string("Attesa una parentesi aperta") );
					}
					j++;
					while ( right[j] && right[j] != ')' ) j++;
					range = right.substr( iniz, j - iniz + 1 )  ;
					j++;
				}
				else if ( right.substr(j, 5) == "const" ) {
					fix = right.substr( j, 5 )  ;
					j += 5;
					while ( isspace(riga[j])  ) j++;

				}
				else if ( !isspace(right[j]) ) {	// se non è const o range deve essere uno space
					for ( i = j+1; i < right.size() && !isspace(right[i]); i++)
						;
					std::string c = right.substr(j, i-j);
					std::ostringstream oss ( "" );
					oss << "Carattere \"" << c << "\" non ammesso nella inizializzazione della variabile " << var;
					throw( oss.str() );
				}

			}

		}
		else {
			value = right;
		}

		// separa var in componente/linea . variabile
		std::string nn;
		std::string vv;
		p = var.find(".");
		if ( p != std::string::npos ) {
			nn = var.substr( 0, p );
			vv = var.substr( p+1 );
		}
		else {
			nn = def;
			vv = var;
		}

		auto it_node = components.find( nn );
		auto it_arc = lines.find( nn );

		Variabile * pv = new Variabile ;

		if ( fix != "" ) {
			pv->fix = true;
		}

		pv->val = atof( value.c_str() );
		pv->name = vv;
		if ( it_node != components.end() && it_arc != lines.end() ) {
			std::ostringstream oss ( "" );
			oss << "Il nodo " << nn << " è ambiguo (esiste sia come componente sia come linea";
			throw( oss.str() );
		}
		else if ( it_node != components.end() ) {
			COMPONENTE * comp = it_node->second;
			auto it = comp->vars.find(vv);
			if ( it != comp->vars.end() ) {

				comp->vars[vv] = pv;

			}
			else {
				if ( !pv->fix ) {
					std::ostringstream oss ;
					oss << "La variabile " << vv << " è già registrata nel componente " << nn << std::endl;
					throw( oss.str() );
				}
			}

		}
		else if ( it_arc != lines.end() ) {
			LINEA * comp = it_arc->second;
			auto it = comp->vars.find(vv);
			if ( it != comp->vars.end() ) {

				comp->vars[vv] = pv;

			}
			else {
				std::ostringstream oss ( "" );
				oss << "La variabile " << vv << " è già registrata nella linea " << nn << std::endl;
				throw( oss.str() );
			}
		}
		else {
			std::ostringstream oss ( "" );
			oss << "Il nodo " << nn << " non esiste";
			throw( oss.str() );
		}

		if ( !pv ) {
			std::ostringstream oss ( "" );
			oss << "La variabile " << nn << "." << vv << " non esiste";
			throw( oss.str() );
		}

/*
		TESPRESSIONE exp;

		try {
			REAL r = exp.Risolvi( value.c_str() );
			*pv = r;
		}
		catch ( string e ) {
			ostringstream oss ( "" );
			oss << "Nella inizializzazione della variabile " << e;
			throw( oss.str() );
		}
*/

		if ( range != "" ) {

			// trova minimi e massimo
			size_t i = 0;
			while ( range[i] != '(' ) i++;
			i++;
			std::string min;
			while ( range[i] != ')' && range[i] != ',' ) {
				min += range[i];
				i++;
			}
			if ( range[i] == ')' ) {
				std::ostringstream oss ;
				oss << "Gli operandi di \"range\" devono essere separati da una virgola";
				throw( oss.str() );
			}

			min = utils::trim( min );
			if ( utils::isNumber ( min ) )
				pv->min = atof( min.c_str() ) ;
			else {
				std::ostringstream oss ( "" );
				oss << "Gli operandi di \"range\" devono essere numerici";
				throw( oss.str() );
			}
			i++;
			std::string max;
			while ( range[i] != ')' ) {
				max += range[i];
				i++;
			}
			max = utils::trim( max );
			if ( utils::isNumber ( max ) ) pv->max = atof( max.c_str() ) ;
			else {
				std::ostringstream oss ( "" );
				oss << "Gli operandi di \"range\" devono essere numerici";
				throw( oss.str() );
			}
		}

	}

	return true;

}

std::string System::getXML ()
{

	XMLDocument doc ;
	XMLElement progetto  ( "progetto" );



	for ( auto it = components.begin(); it != components.end(); it++ )  {

		COMPONENTE * no = it->second;

		XMLElement componente  ( "componente" );
		componente.setAttribute( "ident",  no->item ) ;

		XMLElement equazione  ( "equazione" );
		for ( auto it = no->funzs.begin(); it != no->funzs.end(); it++ ) {

			Funzione * equ = *it;
			XMLText text ( equ->base );
			equazione.appendChild( text );

		}

		componente.appendChild( equazione );

		for ( auto it = no->vars.begin(); it != no->vars.end(); it++ ) {

			Variabile * var = it->second;
			REAL v = *var;
			REAL min = var->min;
			REAL max = var->max;


			XMLElement variabile  ( "variabile" );
			variabile.setAttribute( "ident",  it->first ) ;
			variabile.setAttribute( "val",  FloatToStr( v ) ) ;
			if ( var->fix )
				variabile.setAttribute( "const",  "const" ) ;
			variabile.setAttribute( "min",  FloatToStr( min ) ) ;
			variabile.setAttribute( "max",  FloatToStr( max ) ) ;

			componente.appendChild( variabile );


		}


		// i bocchelli

		for ( auto it = no->nozzles.begin(); it != no->nozzles.end(); it++ ) {


			NOZZLE *nozzle = it->second;
			std::string nome_nozzle = nozzle->item;
			std::string nome_linea = nozzle->linea->item;

			XMLElement noz  ( "nozzle" );
			noz.setAttribute( "ident", nome_nozzle );
			noz.setAttribute( "linea", nome_linea.c_str() );

			componente.appendChild( noz );


		}



		progetto.appendChild( componente );

	}



	for ( auto it = lines.begin(); it != lines.end(); it++ ) {

		LINEA * no = it->second ;

		NOZZLE * in = no->in;
		NOZZLE * out = no->out;

		XMLElement linea  ( "linea" );
		linea.setAttribute( "ident",  no->item ) ;

		std::string s_in, s_out;
		if ( in ) {

			std::ostringstream oss ( "" );
			oss << in->parent->item << "." << in->item   ;
			s_in = oss.str();

			linea.setAttribute( "inlet",  s_in.c_str() ) ;

		}

		if ( out ) {

			std::ostringstream oss ( "" );
			oss <<  out->parent->item << "." << out->item  ;
			s_out = oss.str();

			linea.setAttribute( "outlet",  s_out.c_str() ) ;

		}

		for ( auto it = no->vars.begin(); it != no->vars.end(); it++ ) {

			Variabile * var = it->second;
			REAL v = *var;
			REAL min = var->min;
			REAL max = var->max;


			XMLElement variabile  ( "variabile" );
			variabile.setAttribute( "ident",  it->first ) ;
			variabile.setAttribute( "val",  FloatToStr( v ) ) ;
			if ( var->fix )
				variabile.setAttribute( "const",  "const" ) ;
			variabile.setAttribute( "min",  FloatToStr( min ) ) ;
			variabile.setAttribute( "max",  FloatToStr( max ) ) ;

			linea.appendChild( variabile );

		}

		progetto.appendChild( linea );


	}

	HashClass * var = TIdent::getVariableTable () ;
	HashClass * tipo = TIdent::getTipoIdentificatore () ;

	if ( var ) {

        HashIterator it1 ( var )  ;
        HashIterator it2 ( tipo )  ;

        it1.First();
        while ( !it1.Eof() ) {

            std::string s = it1.GetKey();

            TIdent::Tipo t = *(TIdent::Tipo *)tipo->GetData( s.c_str() ) ;
            switch ( t ) {
                case TIdent::tpTABELLA : {

                    XMLElement tabella  ( "tabella" );
                    tabella.setAttribute( "ident",  s.c_str() ) ;

                    TTAB::TMem * mem = *(TTAB::TMem **) it1.GetData() ;
                    Curva * curva = mem->curva;

                    size_t El = curva->getEl ();	// numero di elementi
                    tabella.setAttribute( "elementi",  IntToStr(El) ) ;
                    for ( size_t i = 0; i < El; i++ ) {

                        Punto p = curva->getPoint( i );	// ritorna il punto i-esimo
                        XMLElement dato  ( "dato" );
                        dato.setAttribute( "x", FloatToStr(p.x) ) ;
                        dato.setAttribute( "y", FloatToStr(p.y) ) ;

                        tabella.appendChild( dato );

                    }

                    progetto.appendChild( tabella );

                }

                    break;

                case TIdent::tpMATRICE :
                    break;

                default :

                    break;

            }


            it1.Next();


        }

	}


	doc.appendChild( progetto );

	return doc.printXML();


}


// implementazione della classe azione
inline System::Equazione::Equazione( class System * sys ) :
	system( sys )
{
}

inline System::Equazione::~Equazione()
{
}

inline REAL & System::Equazione::callback  ( const char * str )
{

	std::string arg = str;

	// nome_equip.grand
	// nome_equip.nome_nozzle.grand

	size_t pos = arg.find ( "." );
	if ( pos == std::string::npos ) {
		std::ostringstream oss ;
		oss << "identificatore " << str << " non trovato" << std::endl;
		throw( oss.str() );
	}
	std::string nome = arg.substr( 0, pos);
	std::string var = arg.substr( pos+1, arg.size() );

	auto it = system->components.find( nome );
	if ( it != system->components.end() ) {

		COMPONENTE *c = it->second ;

		// verifica se è un bocchello (in questo caso c'è un altro punto
		pos = var.find ( "." );
		if ( pos == std::string::npos ) {

			// è una variabile del componente
			auto it = c->vars.find( var );
			if ( it != c->vars.end() ) {

				REAL &ret_val = it->second->GetRif();
				return ret_val;

			}
			else {

				std::stringstream ss ;
				ss << "Variabile "  << var << " non presente nel componente " << nome << std::endl;
				throw ( ss.str() ) ;

			}

		}
		else {

			std::string nome_nozzle = var.substr( 0, pos);
			std::string v = var.substr( pos+1 );

			// si tratta di un bocchello
			auto it = c->nozzles.find ( nome_nozzle );
			if ( it != c->nozzles.end() ) {

				NOZZLE * nozzle = it->second;
				LINEA *linea = nozzle->linea;

				if ( linea ) {

					auto it = linea->vars.find ( v );
					if ( it != linea->vars.end() ) {

						REAL &ret_val = it->second->GetRif();
						return ret_val;

					}

				}
				else {

					std::stringstream ss ;
					ss << "Bocchello "  << nome_nozzle << " non collegato ad alcuna linea." << std::endl;
					throw ( ss.str() ) ;

				}

			}
			else {

				std::stringstream ss ;
				ss << "Bocchello "  << nome_nozzle << " non presente nel componente " << nome << std::endl;
				throw ( ss.str() ) ;

			}

		}


	}
	else {

		std::stringstream ss ;
		ss << "Componente " << nome << " non presente" << std::endl;
		throw ( ss.str() ) ;

	}


	static REAL t = 0;

	return t;

}

double System::variable ( const std::string & nome_variabile ) const
{

	// il nome ha la sintassi nome_componente.nome_variabile
	// il nome ha la sintassi nome_linea.nome_variabile
	// o nome_variabile

	// verifica se c'è il punto

	std::size_t found = nome_variabile.find( "."  );
	if ( found != std::string::npos ) {

		// c'è il punto, separa tra componente e variabile
		std::string nome_componente = nome_variabile.substr( 0, found );
		std::string var = nome_variabile.substr( found + 1 );

		auto it = components.find( nome_componente );
		if ( it != components.end() ) {

			COMPONENTE *c = it->second ;

			// è una variabile del componente
			auto it_v = c->vars.find( var );
			if ( it_v != c->vars.end() ) {

				double ret_val = it_v->second->Val();
				return ret_val;

			}

		}

	}
	else {

		for ( auto it = components.begin(); it != components.end(); it++ ) {

			COMPONENTE *c = it->second ;

			auto it_v = c->vars.find( nome_variabile );
			if ( it_v != c->vars.end() ) {

				double ret_val = it_v->second->Val();
				return ret_val;

			}

		}

	}


	std::stringstream ss ;
	ss << "Variabile "  << nome_variabile << " non presente"  << std::endl;
	throw ( ss.str() ) ;


	return 0;
}

std::map< std::string, double > System::mapVariable ( ) const
{

	std::map< std::string, double > vars;
	size_t num_comp = components.size();
	for ( auto it = components.begin(); it != components.end(); it++ ) {

		COMPONENTE *c = it->second ;

		for ( auto jt = c->vars.begin(); jt != c->vars.end(); jt++ ) {
			std::string v ;
			if ( num_comp == 1 )
				v =  jt->first;
			else
				v = c->item + "." + jt->first;

			vars.insert ( std::pair< std::string, double >( v, jt->second->Val()) );
		}
	}

	return vars;

}

std::string  System::plot ( int f_index, int v_index )
{

	Variabile & v = *v_var[v_index];
	Funzione &  funz = * v_funz[f_index];
			
	double min = v.min;
	double max = v.max;
			
	std::ostringstream oss ;
	std::ostringstream x_oss ;
	std::ostringstream y_oss ;
	x_oss << "[";
	y_oss << "[";
	int dot_x = 100;
	double step = (max - min) / dot_x;
	double y_min=1000000, y_max=0;
	for ( int i = 0; i <= dot_x; i++ ) {
		v = min+i * step;
		double y2 = funz();
		if ( i < dot_x ) {
			x_oss << v.Val() << ",";
			y_oss << y2 << ",";
		}
		else {
			x_oss << v.Val() << "]";
			y_oss << y2 << "]";
		}
		if ( y2 < y_min ) y_min=y2;
		if ( y2 > y_max ) y_max=y2;
	}
	
	oss 
		<< "{"
		<< "\"x\" :" << x_oss.str() << ","
		<< "\"y\" :" << y_oss.str() << ","
		<< "\"dot_x\" : " << dot_x  << ","
		<< "\"x_min\" : " << min  << ","
		<< "\"x_max\" : " << max  << ","
		<< "\"y_min\" : " << y_min << ","
		<< "\"y_max\" : " << y_max
		<< "}";
			
	return oss.str();

}

inline void System::Equazione::command  ( const char * )
{
}


