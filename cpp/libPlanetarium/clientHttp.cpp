#include <sstream>
#include <fstream>
#include <thread>         // std::thread

#include <string.h>
#include <stdlib.h>

#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>

#include <cerrno>

#include "clientHttp.hpp"
#include "Utils.h"



Client::Client(  ) :
	extra ( nullptr )
{
	connected_timer.start();
}

void Client::setQueryString( const std::string & query )
{

	the_query_string = query;
	std::vector< std::string > v;

	Split ( query, "&", v );

    for ( auto it = v.begin(); it != v.end(); ++it ) {
        std::vector < std::string > v1;
        int n = Split ( *it, "=", v1);
        if (n > 0)
            query_string.insert ( pairQuery ( v1[0], v1[ 1 ] ) );
    }

}

std::string Client::getQueryString ( const std::string & key ) const
{

        auto it = query_string.find( key );
        if ( it != query_string.end() )
                return it->second;
        else return "";

}

std::string Client::getQueryString ( ) const
{
        return the_query_string;
}

void Client::addHeader( std::string h )
{

	std::vector< std::string > v1;
	int n = Split ( h, ":", v1);

	if (n > 0) {
		header[ v1[0] ] = v1[1] ;
	}

}

std::string Client::getHeaderContent( const std::string & key ) const
{

        auto it = header.find( key );
        if ( it != header.end() )
                return it->second;
        else return "";

}

std::string Client::getCookieContent( const std::string & key ) const
{

    // ricava dell'header i cookies
    std::string cookies = getHeaderContent( "Cookie" ) ;
    size_t pos = cookies.find ( key );
    if ( pos != std::string::npos ) {
    }

    return cookies;

}

size_t Client::Split (const std::string& input, const std::string& delimiter, std::vector< std::string > &v) const
{

	size_t res = 0;
	size_t iniz = 0;
	size_t n = input.find(delimiter);

	while (n != std::string::npos) {

		v.push_back(input.substr(iniz, n-iniz));
		iniz = ++n;
		res++;
		n = input.find(delimiter, iniz);

	}

	v.push_back(input.substr(iniz, input.size()));


	return res;

}
