#include <iostream>
#include <iterator>
#include <list>
#include <stdio.h>
#include <stdlib.h>

#include "tmysql.hpp"

using namespace std;
int main()
{
	
        
	TConnection connessione ;

	connessione.Host = "www.mondobimbi.com";
	connessione.User = "sergio";
	connessione.Password = "eralca889190";
	connessione.Db = "gestionale";

	try {
		connessione.Active = true;
	}
	catch ( string e ) {
		cout << e << endl;
		exit (1) ;
	}

//	TQuery q ("localhost", "root", "", "gestionale") ;
	TQuery q = connessione ;

	q.Add("select codice_merce as codice, left(descrizione, 20) as descrizione, format(prezzo_di_vendita_web/100,2) as prezzo");
	q.Add("from merci");
	q.Add("where codice_fornitore=\"PLA\"");

	try {

		q.Open();

		q.First ();
		while ( !q.Eof() ) {

			string codice = q.FieldByName ( "codice" );
			string descrizione = q.FieldByName ( "descrizione" );
			std::string  prezzo = q.FieldByName ( "prezzo" );
			cout <<  codice << " " << descrizione << " " << prezzo << endl;
			q.Next () ;

		}

		q.Close () ;

	}
	catch ( string e ) {
		cout << e << endl;
	}


/*
        TmCampo nome ;
        nome=true;
        
        TmQuery query("localhost","root","","gestionale");
        query.Add("select * from merci");
        query.Add("where codice_fornitore=\"PLA\"");

        query.Open();
        query.First();
        while (!query.Eof()) {
                string codice = query.Field("codice_merce") ;
                string descrizione = query.Field("descrizione") ;
                cout << codice << " " << descrizione << endl;
                query.Next();
        }
*/
}

