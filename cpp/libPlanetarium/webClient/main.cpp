
#include<tcp_client.hpp>

#ifndef __xml__
	#include <xml.hpp>
#endif

int main (int /* argc */, char ** /* argv[] */ )
{

    TCPClient c( "api.geonames.org" );
	//send some data
//    c.require_data( "/timezone", "lat=45.1859&lng=9.1566&username=baldoweb" );
    std::string data = c.get ( "/findNearbyPlaceName", "lat=45.1859&lng=9.1566&username=baldoweb" );


    std::cout << data << std::endl;
    //done
					XMLDocument doc;
					doc.parse( data );
					XMLElement * root = doc.rootElement ()  ; 
				
					XMLElement * geoname =  root->getFirstElementByTagName ( "geoname" ) ;
					XMLElement * name =  geoname->getFirstElementByTagName ( "name" ) ;
					std::string n = name->innerHTML();
    return 0;




}
