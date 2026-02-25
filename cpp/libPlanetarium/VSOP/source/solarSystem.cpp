#include <sstream>

#ifndef __xml__
	#include <xml.hpp>
#endif

#include <Utils.h>
#include <solarSystem.hpp>

SolarSystem::SolarSystem ( const std::string & data ) 
{
	

	XMLDocument doc ;
	if ( data == "" ) {

		// se è vuoto usa il default
		try {

			std::stringstream dt;
			dt << 
				"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
				"<body />";
			doc.loadFromString ( dt.str() );

		}
		catch ( const std::string & str ) {

			throw ( str ) ;

		}
	
	}
	else if ( data.substr(0,5) != "<?xml" ) {


		try {
		
			// prova a caricarlo da file
			doc.loadFromFile ( data );

		}
		catch ( const std::string & str ) {

			throw ( str ) ;

		}
		
	}
	else {

		// se non è file prova da stringa
		try {

			doc.loadFromString ( data );

		}
		catch ( const std::string & str ) {

			throw ( str ) ;

		}

	}

	XMLElement * root = doc.rootElement();
	map_dir = utils::trim(root->getAttribute("map_dir"));

	std::string name = root->getAttribute("name");
	std::string map = root->getAttribute("map");
	std::string shader = root->getAttribute("shader");
	std::string side = root->getAttribute("side");
	
	if ( name == "earth" ) {
	
		earth.setMap( map_dir, map );
	
		if ( shader != "" )
			earth.gl.shader = shader;
	
		if ( side != "" )
			earth.gl.side = side;
	
		earth.parent = nullptr;
		sky.parent = nullptr;
	
		addBody ( root, &earth ) ;
	
	}

	moon.parent = &earth ;

	io.parent = &jupiter;
	europa.parent = &jupiter;
	ganimede.parent = &jupiter;
	callisto.parent = &jupiter;

	bodies.insert ( std::pair< std::string, CelestialBody & >( "sky", sky ) );
	bodies.insert ( std::pair< std::string, CelestialBody & >( "sun", sun ) );
	bodies.insert ( std::pair< std::string, CelestialBody & >( "earth", earth ) );
	bodies.insert ( std::pair< std::string, CelestialBody & >( "moon", moon ) );
	bodies.insert ( std::pair< std::string, CelestialBody & >( "mercury", mercury ) );
	bodies.insert ( std::pair< std::string, CelestialBody & >( "venus", venus ) );
	bodies.insert ( std::pair< std::string, CelestialBody & >( "mars", mars ) );
	bodies.insert ( std::pair< std::string, CelestialBody & >( "jupiter", jupiter ) );
	bodies.insert ( std::pair< std::string, CelestialBody & >( "io", io ) );
	bodies.insert ( std::pair< std::string, CelestialBody & >( "europa", europa ) );
	bodies.insert ( std::pair< std::string, CelestialBody & >( "ganimede", ganimede ) );
	bodies.insert ( std::pair< std::string, CelestialBody & >( "callisto", callisto ) );
	bodies.insert ( std::pair< std::string, CelestialBody & >( "saturn", saturn ) );
	bodies.insert ( std::pair< std::string, CelestialBody & >( "uranus", uranus ) );
	bodies.insert ( std::pair< std::string, CelestialBody & >( "neptune", neptune ) );

}


SolarSystem::~SolarSystem()
{
}

void SolarSystem::addBody ( XMLElement * root, CelestialBody * parent ) 
{
	
	for ( XMLNode * n = root->firstChild(); n; n = n->nextSibling() ) {

		XMLElement * el = dynamic_cast<XMLElement *>(n);

		std::string tag = el->value();
		if ( tag == "body" ) {

			CelestialBody * child;
			std::string name = el->getAttribute("name");
			std::string map = utils::trim(el->getAttribute("map"));
			std::string side = root->getAttribute("side");
			
	
			std::string shader = el->getAttribute("shader");
			
			if ( name == "sun" ) {
			
				child = & sun;
			
			}
			else if ( name == "sky" ) {
			
				child = & sky;
			
			}
			else if ( name == "mercury" ) {
			
				child = & mercury ;
			
			}
			else if ( name == "venus" ) {
			
				child = & venus;
			
			}
			else if ( name == "earth" ) {
			
				child = & earth;
			
			}
			else if ( name == "moon" ) {
			
				child = & moon;
			
			}
			else if ( name == "mars" ) {
			
				child = & mars ;
			
			}
			else if ( name == "jupiter" ) {
			
				child = & jupiter ;
			
			}
			else if ( name == "io" ) {
			
				child = & io ;
			
			}
			else if ( name == "europa" ) {
			
				child = & europa ;
			
			}
			else if ( name == "ganimede" ) {
			
				child = & ganimede ;
			
			}
			else if ( name == "callisto" ) {
			
				child = & callisto ;
			
			}
			else if ( name == "saturn" ) {
			
				child = & saturn;
			
			}
			else if ( name == "uranus" ) {
			
				child =  & uranus;
			
			}
			else if ( name == "neptune" ) {
			
				child =  & neptune;
			
			}
	
	
			child->setMap( map_dir, map );
			
			if ( shader != "" )
				child->gl.shader = shader;
			if ( side != "" )
				child->gl.side = side;

			child->parent = parent;
				
			parent->childs.push_back ( child );
				
			addBody ( el, child );
			

		}
		
	}


}

std::string SolarSystem::primitive () const 
{
	
	std::stringstream res;
	for ( auto it = bodies.begin(); it != bodies.end(); it++ ) {
		
		CelestialBody & body = it->second;
		res << body.primitive();
	
	}
	
	return res.str();
	
}

std::string SolarSystem::primitive_pos () const 
{
	
	std::stringstream res;
	for ( auto it = bodies.begin(); it != bodies.end(); it++ ) {
		
		CelestialBody & body = it->second;
		res << body.primitive_pos();
	
	}
	
	return res.str();
	
}



