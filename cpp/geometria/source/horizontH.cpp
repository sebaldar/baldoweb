#include <sstream>

#include <celestial_body.hpp>
#include <horizont.hpp>
#include <line.hpp>
#include <plane.hpp>


Horizont::Horizont ( double _latitude, double _longitude, double H ) :
	latitude( _latitude), longitude( _longitude )
{

						
	Earth earth;
	Point coord = earth.bodyCoord ( latitude, longitude, H );

	double xc = coord.x / CelestialBody::AU;
	double yc = coord.y / CelestialBody::AU;
	double zc = coord.z / CelestialBody::AU;
	
	x_pdv = xc;
	y_pdv = yc;
	z_pdv = zc;
	
	// punto di vista
	pdv = Point ( xc, yc, zc );

	// retta ortogonale al punto di osservazione
	StraightLine ort_pdv ( Point( 0, 0, 0 ), pdv );


	// cono sull'orizzonte
	// semiangolo del vertice dell cono
	// (R+H) * sin (alfa) = R
	double R = earth.radius;	// km
	
	Radiant alfa = asin( R / ( R + H / 1000 ) );
	
	// raggio di base del cono
	// /r_cono / ( R+H) = tg(alfa)
	r_cono ( R + H / 1000 ) * tg( alfa );
	h_cono = R + H / 1000;





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
			
	Earth earth;
	double R = earth.radius;
	
	// coordinata z punto di osservazione
	double 

	Gradi azm = azimut( p );
	// le coordinate x, y del punto di tangenza
	Radiant r_azm = azm;
	double x = R * cos( r_azm );
	double y = R * sin( r_azm );
	
	// retta ortogonale al punto di vista
	StraightLine ort_pdv ( Point( 0, 0, 0 ), pdv );

	StraightLine lookat ( pdv, p );
	Radiant angle = ort_pdv.angle ( lookat ) ;

	return 90 - Grade(angle);

}

std::string Horizont::track(bool show)
{
    // Costanti utili
    const double PI = atan(1.0) * 4.0;
    const double TWOPI = 2.0 * PI;

    // Normalizziamo la direzione zenit (verso l'alto dal punto di vista)
    Direction zenit = pdv.normalized();  // assume che Direction abbia .normalized()

    // Troviamo due vettori perpendicolari tra loro e perpendicolari a zenit
    // (base ortonormale locale: est + nord)
    Direction nord_polare(0, 0, 1);  // asse z = nord celeste

    Direction est;
    if (std::abs(zenit.z) > 0.99999) {
        // Vicino ai poli → caso speciale
        est = Direction(1, 0, 0);
    } else {
        est = zenit.cross(nord_polare).normalized();
    }

    Direction nord = est.cross(zenit).normalized();

    // Raggio del cerchio dell’orizzonte
    // Per osservatore sulla superficie → leggermente più grande del raggio terrestre
    Earth earth;
    double r_terra = earth.a / CelestialBody::AU;           // raggio in UA
    double r_horizon = r_terra * 1.0005;                    // piccolo margine per visibilità

    // Stringa di output
    std::string is_hidden = show ? "<show />" : "<hidden />";

    std::stringstream ss;
    ss << "<primitive type=\"line\" size=\"1\" name=\"hor_circle\">" << std::endl;
    ss << is_hidden << std::endl;
    // ss << "<recreate />" << std::endl;   // decommenta se serve

    // Generazione dei 60–72 punti del cerchio (più punti = più fluido)
    const int steps = 72;
    for (int i = 0; i <= steps; ++i) {
        double angle = TWOPI * static_cast<double>(i) / static_cast<double>(steps);

        double cx = cos(angle);
        double sx = sin(angle);

        // Costruiamo il punto nel piano orizzontale locale
        double x = (cx * est.x + sx * nord.x) * r_horizon;
        double y = (cx * est.y + sx * nord.y) * r_horizon;
        double z = (cx * est.z + sx * nord.z) * r_horizon;

        ss << "<vertice>" << x << " " << y << " " << z << "</vertice>" << std::endl;
    }

    ss << "<lookat x=\"" << pdv.x << "\" y=\"" << pdv.y << "\" z=\"" << pdv.z << "\" />" << std::endl;
    ss << "<position x=\"" << pdv.x << "\" y=\"" << pdv.y << "\" z=\"" << pdv.z << "\" />" << std::endl;
    ss << "<material type=\"line_basic\" size=\"2\" color=\"0x12cff8\" />" << std::endl;
    ss << "</primitive>" << std::endl;

    // ───────────────────────────────────────────────
    // Linea verso l'orizzonte (opzionale – come prima)
    // ───────────────────────────────────────────────
    ss << "<primitive type=\"line\" size=\"1\" name=\"hor_line\">" << std::endl
       << is_hidden << std::endl
       << "<recreate />" << std::endl
       << "<vertice>" << pdv.x << " " << pdv.y << " " << pdv.z << "</vertice>" << std::endl;

    // Direzione qualsiasi verso l’orizzonte (es. est)
    Direction verso_orizzonte = est * (r_terra * 0.02);  // lunghezza arbitraria visibile

    ss << "<vertice>"
       << (pdv.x + verso_orizzonte.x) << " "
       << (pdv.y + verso_orizzonte.y) << " "
       << (pdv.z + verso_orizzonte.z)
       << "</vertice>" << std::endl;

    ss << "<material type=\"line_basic\" size=\"1\" color=\"0x12cff8\" />" << std::endl
       << "</primitive>" << std::endl;

    // ───────────────────────────────────────────────
    // Linea nord-sud locale (opzionale)
    // ───────────────────────────────────────────────
    ss << "<primitive type=\"line\" size=\"1\" name=\"hor_nord\">" << std::endl
       << is_hidden << std::endl
       << "<recreate />" << std::endl
       << "<vertice>" << pdv.x << " " << pdv.y << " " << pdv.z << "</vertice>" << std::endl
       << "<vertice>"
       << (pdv.x + nord.x * r_terra * 0.02) << " "
       << (pdv.y + nord.y * r_terra * 0.02) << " "
       << (pdv.z + nord.z * r_terra * 0.02)
       << "</vertice>" << std::endl
       << "<material type=\"line_basic\" size=\"1\" color=\"0x12cff8\" />" << std::endl
       << "</primitive>" << std::endl;

    return ss.str();
}
