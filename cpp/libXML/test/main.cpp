#include <iostream>

#include "xml.hpp"

int main (int argc, char **argv)
{



	std::string nome_file = "./test.xml";
	XMLDocument doc;
	try {

		doc.loadFromFile ( nome_file );

	}
	catch ( std::string str ) {

		std::cout << "Errore da main : " << str << std::endl;

	}

	std::cout << doc.printXML () << std::endl;

	return ( 0 );

}
