#include <sstream>

#include <celestial_body.hpp>
#include <horizont.hpp>
#include <line.hpp>
#include <plane.hpp>


Horizont::Horizont ( double _latitude, double _longitude ) :
	latitude( _latitude), longitude( _longitude )
{

						
	Earth earth;
	Point coord = earth.bodyCoord ( latitude, longitude, H );

	double xc = coord.x / CelestialBody::AU;
	double yc = coord.y / CelestialBody::AU;
	double zc = coord.z / CelestialBody::AU;
	
	// punto di vista
	pdv = Point ( xc, yc, zc );

	// retta ortogonale al punto di vista
	StraightLine ort_pdv ( Point( 0, 0, 0 ), pdv );

	// piano orizzonte
	horizont = Plane ( ort_pdv, pdv );


	// intersezione piano tangente a coord e asse z
	double z = ( xc * xc + yc * yc + zc * zc ) / zc;

	// punto su asse z e appartente a horizont
	p_north = Point ( 0, 0, z );

	// distanza tra p_z e pdv
	double dz = Point::distance( pdv, p_north);

	// linea nord-sud
	line_south_north = StraightLine( pdv,  p_north );

	// calcolo linea west-east
	{
	
		Point coord = earth.bodyCoord ( latitude, longitude + DELTA, H );

		double x = coord.x / CelestialBody::AU;
		double y = coord.y / CelestialBody::AU;
		double z = coord.z / CelestialBody::AU;

		// linea nuova posizione
		StraightLine intersect (  Point( 0, 0, 0 ), Point( x, y, z ) );
		// intersezione piano orizzonte con linea intesect
		Point p_east_rif =  StraightLine::intersect ( intersect, horizont ) ;
	
		// linea ovest-east
		line_west_east = StraightLine ( pdv,  p_east_rif );

	}
	// fine calcolo linea west-east



			
	// linea sud-nord
	line_north_south = StraightLine ( line_south_north.direction().oppost(),  pdv );
	// linea east a ovest
	line_east_west = StraightLine ( line_west_east.direction().oppost(),  pdv );

	// punto a east distante dz
	p_east = Point ( line_west_east.pointAtDistance( dz ) );
	// punto a sud distante dz
	p_south = Point ( line_north_south.pointAtDistance( dz ) );
	// punto a ovest distante dz
	p_west = Point ( line_east_west.pointAtDistance( dz ) );


}

Point Horizont::height_correction ( const Point & ph, double height) const
{
			
	// direzione ortogonale all'orizzonte
	Direction d = horizont.direction();

	StraightLine line ( d, ph );
			
	// distanza da z a pdv
	double r = pdv.distance( ph );
			
	// altezza su orizzonte
	Radiant alfa = Grade( height );
	double ro = r * tan( alfa );
			
	return line.pointAtDistance ( ro ); 
	
}

Point Horizont::lookatAzimut ( double azimut, double altezza ) const 
{
						

	if ( azimut >= 0 && azimut <= 90 ) {


		Point ph = Point::intermediate ( p_north, p_east, azimut * 100 / 90 );
			
		return ( height_correction ( ph, altezza ) );
						
	}
	else if ( azimut > 90 && azimut <= 180 ) {

		Point ph = Point::intermediate ( p_east, p_south, ( azimut - 90 ) * 100 / 90 );
			
		return ( height_correction ( ph, altezza ) );
					
	}
	else if ( azimut > 180 && azimut <= 270 ) {

		Point ph = Point::intermediate ( p_south, p_west, ( azimut - 180 ) * 100 / 90 );
			
		return ( height_correction ( ph, altezza ) );

						
	}
	else if ( azimut > 270 && azimut <= 360 ) {

		Point ph = Point::intermediate ( p_west, p_north, ( azimut - 270 ) * 100 / 90 );

		return ( height_correction ( ph, altezza ) );
						
	}
	
	// non arriva mai qui
	return Point( 0, 0, 0 );

}

double Horizont::azimut ( const Point & p ) 
{

	// calcola l'azimut
	// punto proiezione di p su horizont
	StraightLine line ( horizont.direction(), p );
	// punto di intersezione tra piano orizzonte e line
	Point intersect = StraightLine::intersect( line, horizont ) ;

	StraightLine p_line ( pdv, intersect );
				
	// valuta la distanza da punto east e ovest
	double d_e = Point::distance( intersect, p_east );
	
	double d_w = Point::distance( intersect, p_west );
	
	Radiant angle = StraightLine::angle ( p_line, line_south_north );
	if ( d_e < d_w ) {
		// si trova a east
		return Grade( angle );
	}
	else {
		// si trova a ovest
		return 360 - Grade( angle );
	}
	

}

double Horizont::height ( const Point & p ) 
{
			
	// retta ortogonale al punto di vista
	StraightLine ort_pdv ( Point( 0, 0, 0 ), pdv );

	StraightLine lookat ( pdv, p );
	Radiant angle = ort_pdv.angle ( lookat ) ;

	return 90 - Grade(angle);

}

std::string Horizont::track ( bool show ) 
{
			
	Direction dx ( 1, 0, 0  );
	Direction dy ( 0, 1, 0  );
	Direction dz ( 0, 0, 1  );
				
	Direction d ( pdv );

	Grade alfa_x = Radiant( StraightLine::angle( d, dx ) ) ;
	Grade alfa_y = Radiant( StraightLine::angle( d, dy ) ) ;
	Grade alfa_z = Radiant( StraightLine::angle( d, dz ) ) ;
				
	std::string is_hidden = "<hidden />" ;
	if ( show ) {
		is_hidden = "<show />" ;
	}
	
	Earth earth;
	double r = earth.a / CelestialBody::AU;

	std::stringstream track_horizont ;


	track_horizont << "<primitive type=\"line\" size=\"1\" name=\"hor_circle\">"  << std::endl <<
//		"<recreate />" <<
		is_hidden <<
		std::endl;
					
	double pi = atan( 1 ) * 4 ;
	for ( double i = 0; i <= 2 * pi; i += pi / 30 ) {
				
		double x = r * sin( i );
		double y = r * cos( i );
		double z = 0;
					
		track_horizont << "<vertice>" << x << " " << y << " " << z << "</vertice>"  << std::endl;
				
	}

	track_horizont <<
		"	<lookat x=\"" << pdv.x << "\" y=\"" << pdv.y << "\" z=\"" << pdv.z << "\" />"	<<
		"	<position x=\"" << pdv.x << "\" y=\"" << pdv.y << "\" z=\"" << pdv.z << "\" />"	<<
		"<material type=\"line_basic\" size=\"2\" color=\"0x12cff8\" />" <<
		"</primitive>" <<
		std::endl;
		
	track_horizont << 
		"<primitive type=\"line\" size=\"1\" name=\"hor_line\">"  << std::endl <<
		is_hidden <<
		"<recreate />" <<
		"<vertice>" << pdv.x << " " << pdv.y << " " << pdv.z << "</vertice>"  << 
		"<vertice>" << pdv.x + horizont.O.x << " " << pdv.y + horizont.O.y << " " << pdv.z + horizont.O.z << "</vertice>"  << 
		"<material type=\"line_basic\" size=\"1\" color=\"0x12cff8\" />" <<
		"</primitive>" <<
		"<primitive type=\"line\" size=\"1\" name=\"hor_nord\">"  << std::endl <<
		is_hidden <<
		"<recreate />" <<
		"<vertice>" << pdv.x << " " << pdv.y << " " << pdv.z << "</vertice>"  << 
		"<vertice>" << pdv.x + line_south_north.D.x << " " << pdv.y + line_south_north.D.y << " " << pdv.z + line_south_north.D.z << "</vertice>"  << 
		"<material type=\"line_basic\" size=\"1\" color=\"0x12cff8\" />" <<
		"</primitive>" ;
		
/*	
	track_horizont << 
		"<primitive name=\"hor_circle\" type=\"circle\" radius=\"" << r << "\" " <<
		"segments=\"" << 32 << "\">" <<
		"	<lookat x=\"" << pdv.x << "\" y=\"" << pdv.y << "\" z=\"" << pdv.z << "\" />"	<<
		"	<position x=\"" << pdv.x << "\" y=\"" << pdv.y << "\" z=\"" << pdv.z << "\" />"	<<
		is_hidden <<
		"<recreate />" <<
		"<material type=\"basic\" side=\"double\" color=\"0xffff00\" />" <<
		"</primitive>";
*/

	return track_horizont.str() ;


}

