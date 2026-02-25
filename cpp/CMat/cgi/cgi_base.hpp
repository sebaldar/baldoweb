#include <iostream>
#include <fstream>
#include <list>
#include <map>


class CGI
{

protected:

	std::string document_root;
	std::string method;

	long int length;
	std::string the_query_string ;

	std::list<std::string> fHeader;
	std::list<std::string> fBody;

	std::map<std::string, std::string> query_map;

public:

	enum TIPOFORM
	{
		STANDARD,
		DB
	};


 	CGI ();
	CGI ( const std::string & doc_root, const std::string & met, const std::string & q_string, long int len );
	CGI (  int argc, char **argv  );
	CGI ( const std::string & tag );

	virtual ~CGI ( ) {}

	std::string value ( const std::string & key ) const ;
	std::string query_string ( const std::string & key ) const ;

	std::string query_string ( ) const {
		return the_query_string ;
	}

	void get_query_string ( std::string & query ) {
		the_query_string = query ;
	}

	void header  (   const std::string & type,   const std::string & value );
	void body (   const std::string &  row);

	virtual void print ();

};


