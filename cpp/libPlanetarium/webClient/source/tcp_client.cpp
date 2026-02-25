/**
    C++ client
*/

#include <iostream>
#include <sstream>
#include <vector>

#include <sys/socket.h>    //socket
#include <arpa/inet.h> //inet_addr
#include <netdb.h> //hostent

#include <tcp_client.hpp>

TCPClient::TCPClient() :
	sockfd( -1 ),
	host( "" ),
	port( 0 )
{
}

TCPClient::TCPClient( const std::string & host, int port ) :
	TCPClient ()
{
	connection ( host, port );
}

/**
    Connect to a host on a certain port number
*/
bool TCPClient::connection ( const std::string & address , unsigned int port)
{

    host = address;
    //create socket if it is not already created
    if( sockfd == -1 )
    {

		if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
			throw ("[!!] Errore Fatale - in socket.");


    }
    else    {   /* OK , nothing */  }

	struct sockaddr_in server;
    //setup address structure
    if ( inet_addr( address.c_str() ) == INADDR_NONE ) {

        struct hostent *he;
        struct in_addr ** addr_list;

        //resolve the hostname, its not an ip address
        if ( ( he = gethostbyname( address.c_str() ) ) == NULL) {
            //gethostbyname failed
 			throw ("[!!] Failed to resolve hostnamet.");
        }

        //Cast the h_addr_list to in_addr , since h_addr_list also has the ip address in long format only
        addr_list = (struct in_addr **) he->h_addr_list;

        for( int i = 0; addr_list[i] != NULL; i++ )
        {
            //strcpy(ip , inet_ntoa(*addr_list[i]) );
            server.sin_addr = *addr_list[i];

//            cout<<address<<" resolved to "<<inet_ntoa(*addr_list[i])<<endl;

            break;
        }
    }
    //plain ip address
    else  {
        server.sin_addr.s_addr = inet_addr( address.c_str() );
    }

    server.sin_family = AF_INET;
    server.sin_port = htons( port );

    //Connect to remote server
    if ( connect(sockfd , (struct sockaddr *)&server , sizeof(server)) < 0 )
    {
		std::cout <<  host << " " << port << ":" << "[!!] connect failed. Error." << std::endl;
//		throw ("[!!] connect failed. Error.");
    }

	return true;

}

/**
    Send data to the connected host
*/
bool TCPClient::require_data( const std::string & resorce ) const
{

 	const char * buffer = resorce.c_str();

	int bytes_to_send = resorce.size();
	while ( bytes_to_send > 0) {

		int sent_bytes = send( sockfd, buffer, bytes_to_send, 0 );
		if(sent_bytes == -1)
			return false ; //  send error
		bytes_to_send -= sent_bytes;
		buffer += sent_bytes;

	}

	return true; //  on success

}

/**
    Receive data from the connected host
*/
bool TCPClient::receive( )
{

	char ptr;
	int eol_matched = 0;

	header.clear();
	// prima leggiamo l'header fino a un doppio ritorno carrello
	std::string dest_buffer = "";
	while( recv( sockfd, &ptr, 1, 0 ) == 1 ) { // read a single byte

		if( ptr == EOL[eol_matched] ) { // does this byte match terminator

			eol_matched++;
			if(eol_matched == EOL_SIZE) { // if all bytes match terminator,
//				dest_buffer = dest_buffer.substr( 0, dest_buffer.size() -1 ); // toglie il primo caratteri di EOL
				break ;
			}
			else if ( eol_matched == 2 ) {	// un solo ritorno carrello memorizza nell'header

				dest_buffer = dest_buffer.substr( 0, dest_buffer.size() -1 ); // toglie il primo caratteri di EOL

                std::size_t found = dest_buffer.find( ":" );


                if ( found != std::string::npos ) {
                    std::string name = dest_buffer.substr( 0, found );
                    std::string value = dest_buffer.substr( found + 1  );
                    header.insert( std::pair< std::string, std::string >( name, value ) );

                }
				else {

                    responce = dest_buffer ;

				}

				dest_buffer.clear();
				continue;

			}

		} else {

			eol_matched = 0;

		}

		dest_buffer += ptr; // increment the pointer to the next byte;

	}

	content.clear();
	while( recv( sockfd, &ptr, 1, 0 ) == 1 ) { // read a single byte

		content += ptr; // increment the pointer to the next byte;

	}

	return content.size();

}

std::string TCPClient::get( const std::string & resorce, const std::string & query ) 
{

    std::stringstream ss ;
    ss << "GET " << resorce << 
		"?" << query << " HTTP/1.1\r\nHost: " << host <<
        "\r\nConnection: close" << EOL ;

	require_data( ss.str() ) ;
	receive( ) ;
	
	return content; 

}

std::string TCPClient::allOut () const {

    std::stringstream ss ;
    ss << responce << std::endl;
    for ( auto it = header.begin(); it != header.end(); ++it ) {
        ss << it->first << ":" << it->second << std::endl;
    }
    ss << content;

    return ss.str();
}
