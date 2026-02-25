#include <iostream>
#include <iterator>
#include <list>
#include <stdio.h>
#include <stdlib.h>

#include "tmysql.hpp"

using namespace std;
int main()
{
	
 			std::cout << "Forza la creazione del database ..." << std::endl;

	TConnection connessione ;

	connessione.Host = "127.0.0.1";
	connessione.User = "root";
	connessione.Password = "eralca889190";
	connessione.Db = "";

	try {
		connessione.Active = true;
	}
	catch ( string e ) {
		cout << e << endl;
		exit (1) ;
	}
/*
			TQuery q ( connessione ) ;

			std::cout << "create database if not exists solar" << std::endl;
			q.Add("create database if not exists solar");
			q.Open();
			q.Close () ;

			// revoca i privilegi per poi redarli
			q.Add("REVOKE ALL PRIVILEGES ON solar.* FROM 'sergio'@'localhost'");
			q.Open();
			q.Close () ;

			q.Add("grant all privileges ON solar.* to 'sergio'@'localhost' identified by 'eralca889190'");
			q.Open();
			q.Close () ;
*/
			std::cout << "Fine creazione del del database" << std::endl;

}

