#include <iostream>
#include <fstream>
#include <vector>

#include <string.h>	// per strstr


#include "cgi_base.hpp"

#ifndef until_h
	#include "Utils.h"
#endif


// funzioni di interfaccia
std::string getQueryString ( )
{

	CGI cgi ;
	return cgi.query_string();

}
// fine funzioni di interfaccia


/* urldecode - v. RFC 1738 per i dettagli */
std::string urldecode (   const std::string & input)
{
        std::string str ;
        char c, c1, c2;
        const char *in = input.c_str();
        while ( (c = *in) ) {
                if( c == '%' ) {
                        c1 = *(++in);
                        c2 = *(++in);
//                        if( c1 == EOF || c2 == EOF )  exit(0);
                        c1 = tolower(c1);
                        c2 = tolower(c2);
                        if( ! isxdigit(c1) || ! isxdigit(c2) )  return str;
                        if( c1 <= '9' )
                                c1 = c1 - '0';
                        else
                                c1 = c1 - 'a' + 10;
                        if( c2 <= '9' )
                                c2 = c2 - '0';
                        else
                                c2 = c2 - 'a' + 10;
                        str +=  16 * c1 + c2 ;
                }
                else if( c == '+' )
                        str +=  ' ' ;
                else
                        str +=  c ;
                in++;
        }
        return str;
}

CGI::CGI (  )
{

		std::string s1;
		while ( !std::cin.eof() ) {
			char str;
			std::cin.get ( str );
			if ( std::cin.eof() ) break;
			the_query_string += str;
		}


}


CGI::CGI (  int argc, char **argv  )
{

	/* riceve l'input come argomento */
	for ( int i = 1; i < argc; i++ )
		the_query_string += argv[ i ] + std::string(" ");

	the_query_string = urldecode(the_query_string);

}

CGI::CGI( const std::string & tag )
{


	if ( getenv( "DOCUMENT_ROOT" ) )
		document_root = getenv( "DOCUMENT_ROOT" );

	if ( getenv( "REQUEST_METHOD") )
		method = getenv( "REQUEST_METHOD" );

	if ( getenv( "CONTENT_LENGTH" ) )
		length = atoi( getenv( "CONTENT_LENGTH" ) );
	else length = 0;

	the_query_string = "";
	if ( method == "POST" ) {

		if ( length > 0 ) {

		  char * str = new char [length];

		  std::cin.read (str, length);

		  if (tag != "") {

			  // estrae solo trai tag
			  std::string tag_iniz = "<" + tag;
			  std::string tag_fin = "</" + tag + ">";

			  char *str1 = strstr( str, tag_iniz.c_str() );
			  char *str2 = strstr( str, tag_fin.c_str() );

			  *( str2 + tag_fin.size() ) = '\0';

		  	  the_query_string = str1;
		  }
		  else
		  	  the_query_string = str;

		  delete [] str;

		}

		the_query_string = urldecode(the_query_string);

	}
	else if ( method == "GET" ) {

		if ( getenv("QUERY_STRING") )
			the_query_string = getenv("QUERY_STRING");

		the_query_string = urldecode(the_query_string);

		  if (tag != "") {

			std::string tag_iniz = "<" + tag;
			std::string tag_fin = "</" + tag + ">";

			size_t f1 = the_query_string.find( tag_iniz );
			size_t f2 = the_query_string.find( tag_fin );

			if ( f2 != std::string::npos )
				f2 +=  tag_fin.size () ;
			else
				// ha la forma <tag ... />
				f2 = the_query_string.find( "/>" ) + 2;

			the_query_string = the_query_string.substr( f1, f2 - f1 );

		  }

	}
	else {

		std::string s1;
		while ( !std::cin.eof() ) {
			char str;
			std::cin.get ( str );
			if ( std::cin.eof() ) break;
			the_query_string += str;
		}

	}

}

CGI::CGI( const std::string & doc_root, const std::string & met, const std::string & q_string, long int len ) :
    document_root ( doc_root ), method ( met ), length ( len )
{

	the_query_string = "";
	if ( method == "POST" ) {

		if ( length > 0 ) {

		  char * str = new char [length];

		  std::cin.read ( str, length );

	  	  the_query_string = str;

		  delete [] str;

		}

		the_query_string = urldecode(the_query_string);

	}
	else if ( method == "GET" ) {

		the_query_string = q_string ;

		the_query_string = urldecode(the_query_string);

	}
	else {

		std::string s1;
		while ( !std::cin.eof() ) {

			char str;

			std::cin.get ( str );

			if ( std::cin.eof() ) break;

			the_query_string += str;

		}

	}

}

std::string CGI::query_string ( const std::string & key ) const
{
        const auto it = query_map.find(key);
        if (it != query_map.end())
                return it->second;
        else return "";
}

std::string CGI::value ( const std::string & key ) const {
	return utils::getValue ( the_query_string, key, "&" ) ;
}

void CGI::header (   const std::string &  type,  const std::string & value )
{
	fHeader.push_back(type +":"+value);
}

void CGI::body (   const std::string &  row)
{
	fBody.push_back(row);
}

void CGI::print ()
{

	for ( auto it = fHeader.begin(); it != fHeader.end(); ++it)
		std::cout << *it << std::endl;
	std::cout << "\n";

	for ( auto it = fBody.begin(); it != fBody.end(); ++it)
		std::cout << *it << std::endl;

}


