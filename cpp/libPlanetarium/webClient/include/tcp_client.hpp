#ifndef __tcpclient__
#define __tcpclient__

#include <iostream>    //cout
#include <map>


/**
    TCP Client class
*/
class TCPClient
{
private:


	const std::string EOL = "\r\n\r\n";	// End-Of-Line byte sequence
	const int EOL_SIZE = 4;

    int sockfd;
    std::string host;
	unsigned int port ;	// the port users will be connecting to

	std::map< std::string, std::string > header;
	std::string responce;
	std::string content;

public:

    TCPClient ();
    TCPClient ( const std::string & host, int port = 80 );

    bool connection ( const std::string & , unsigned int );
    bool require_data ( const std::string & resorce ) const ;
    bool receive( )  ;
    std::string get ( const std::string & resorce, const std::string & query )  ;

    std::string allOut () const ;

};


#endif
