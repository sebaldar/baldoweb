#include <math.h>       /* pow */
#include <sstream>
#include <fstream>
#include <exception>      // std::exception
#include <unistd.h>			// usleep
#include <iostream>
#include <iterator>
#include <iomanip>	// setprecision

#include "WSsrv.hpp"
#include "angolo.hpp"
#include "Utils.h"

#include <line.hpp>
#include <horizont.hpp>

#ifndef __xml__
	#include <xml.hpp>
#endif
#include <json.hpp>
#include <curl/curl.h>

#include <map.hpp>
#include <skymap.hpp>
#include <solarSystem.hpp>
#include <solarDb.hpp>

#include <tcp_client.hpp>
#include "clientHttp.hpp"

WSsrv::WSsrv ( ):
	geoLoc ( * new TCPClient( "api.geonames.org") )
{
	initDb();
	print_log("creating maps for moon and earth ...");
	moonGrid = new Map("moon/moon_crater_1");
}


WSsrv::WSsrv ( unsigned int /* port */, std::string & /* web_dir */ ):
	geoLoc ( * new TCPClient( "api.geonames.org") )
{
	print_log("creating maps for moon and earth ...");
	moonGrid = new Map("moon/moon_crater_1");

}

WSsrv::~WSsrv ( )

{
	delete moonGrid;
	delete &geoLoc;
}


void WSsrv::initDb (  )
{
	SolarDb db ;
	print_log("Creating stars ...");
	db.createStars ( );
	print_log("Creating aterims ...");
	db.createAsterism();
	print_log("Creating star index ...");
	db.createStarIndex();

}

// ============================================================
// Implementazione di WSsrv::computeCelestialPositions
// Da inserire in WSsrv.cpp in sostituzione della funzione stub
// ============================================================

std::string WSsrv::computeCelestialPositions ( int sockfd, const std::string &json_input ) {

    // --------------------------------------------------------
    // 1. Parsing del JSON di input
    // --------------------------------------------------------
    JSON json;
    json.loadFromString(json_input);

    std::string s_latitude  = json["latitude"];
    std::string s_longitude = json["longitude"];
    std::string s_height    = json["height"];   // altezza camera in gradi [-90, +90]
    std::string s_azimut    = json["azimut"];   // azimut camera in gradi [0, 360]
    std::string s_date      = json["date"];     // formato "dd-mm-yyyy hh:mm:ss" oppure "dd-mm-yyyy"

    double lat        = atof( s_latitude.c_str()  );
    double lon        = atof( s_longitude.c_str() );
    double cam_height = atof( s_height.c_str()    );  // gradi sull'orizzonte [-90,+90]
    double cam_azimut = atof( s_azimut.c_str()    );  // gradi azimutali [0,360]

    // --------------------------------------------------------
    // 2. Derivazione dell'etichetta cardinale dall'azimut numerico
    //    (usata a scopo descrittivo nell'output JSON)
    //    N=0°, NE=45°, E=90°, SE=135°, S=180°, SW=225°, W=270°, NW=315°
    //    Se altezza >= 85° → Zenith
    // --------------------------------------------------------
    std::string direction_label;
    if ( cam_height >= 85.0 ) {
        direction_label = "Z";
    } else {
        double az = cam_azimut;
        while ( az <   0.0 ) az += 360.0;
        while ( az >= 360.0) az -= 360.0;

        if      ( az <  22.5 )  direction_label = "N";
        else if ( az <  67.5 )  direction_label = "NE";
        else if ( az < 112.5 )  direction_label = "E";
        else if ( az < 157.5 )  direction_label = "SE";
        else if ( az < 202.5 )  direction_label = "S";
        else if ( az < 247.5 )  direction_label = "SW";
        else if ( az < 292.5 )  direction_label = "W";
        else if ( az < 337.5 )  direction_label = "NW";
        else                    direction_label = "N";
    }

    // --------------------------------------------------------
    // 3. Imposta la data sul client e disattiva il loop automatico
    // --------------------------------------------------------
    handleClient( sockfd, "execute_loop off" );

    {
        // Il comando "date" accetta "dd-mm-yyyy hh:mm:ss"
        std::string date_cmd = s_date;
        if ( date_cmd.find(':') == std::string::npos ) {
            date_cmd += " 00:00:00";
        }
        handleClient( sockfd, "date " + date_cmd );
    }

    // --------------------------------------------------------
    // 4. Imposta il punto di osservazione sulla Terra,
    //    poi applica azimut e altezza numerici.
    //    "observe lon lat" senza direzione: posiziona l'osservatore.
    //    "azimut X"  → data->lookat.azimut = X  (gradi 0-360)
    //    "altezza Y" → data->lookat.altezza = Y  (gradi -90/+90)
    //    entrambi impostano VIEW::AZIMUT, usato da set_horizont()
    // --------------------------------------------------------
    {
        std::stringstream cc;
        cc << "observe " << lon << " " << lat;
        handleClient( sockfd, cc.str() );
    }
    {
        std::stringstream cc;
        cc << "azimut " << cam_azimut;
        handleClient( sockfd, cc.str() );
    }
    {
        std::stringstream cc;
        cc << "altezza " << cam_height;
        handleClient( sockfd, cc.str() );
    }

    // --------------------------------------------------------
    // 5. Recupera il puntatore ai dati del client
    // --------------------------------------------------------
    auto it = clients.find( sockfd );
    if ( it == clients.end() ) return "{}";

    Client      * client = it->second;
    clientData  * data   = reinterpret_cast<clientData*>( client->extra );
    if ( !data ) return "{}";

    // --------------------------------------------------------
    // 6. Calcola il giorno giuliano corrente (come in do_sendLoop)
    // --------------------------------------------------------
    short simulation = data->simulation;
    Date  &d         = data->ref_date;
    UT    uut        = d;
    DT    ddt        = uut;
    double JD        = uut.now( simulation );

    // --------------------------------------------------------
    // 7. Calcola posizioni di tutti i corpi celesti
    //    (replica il blocco di do_sendLoop / render)
    // --------------------------------------------------------

    // --- Sole ---
    Sun & sun = data->system->sun;
    OrbitEcli orbit_sun = sun.orbitLBR( JD );
    OrbitEqu  o_sun( JD );
    o_sun = orbit_sun;
    OrbitXYZ  o_sun_xyz = o_sun;
    sun.position = o_sun_xyz;

    // --- Terra (rotazione) ---
    Earth & earth   = data->system->earth;
    double rot       = earth.rotation( ddt, simulation );
    data->rotation   = rot;
    Grade  epsilon   = earth.obliquity( JD );
    earth.obliq      = epsilon;

    OrbitXYZ orbit_earth_xyz( 0, 0, 0 );
    orbit_earth_xyz.rot_y = rot;
    earth.position = orbit_earth_xyz;

    // --- Orbita solare rettangolare (base per i pianeti) ---
    OrbitXYZ orbit_sun_xyz = sun.orbit( MainBody::earth_data, JD );

    // --- Luna ---
    Moon & moon = data->system->moon;
    OrbitEcli orbit_moon = moon.orbitAlfaELP( JD );
    OrbitEqu  o_moon( JD );
    o_moon = orbit_moon;
    OrbitXYZ  o_moon_xyz = o_moon;
    o_moon_xyz.rot_x += 1.543;
    Grade rot_luna = 180 + o_moon.alfa;
    o_moon_xyz.rot_y = rot_luna;
    moon.position = o_moon_xyz;

    // --- Mercurio ---
    Mercury & mercury = data->system->mercury;
    OrbitXYZ orbit_mercury_xyz = mercury.orbit_eq( MainBody::mercury_data, orbit_sun_xyz, epsilon, JD );
    mercury.position = orbit_mercury_xyz;

    // --- Venere ---
    Venus & venus = data->system->venus;
    OrbitXYZ orbit_venus_xyz = venus.orbit_eq( MainBody::venus_data, orbit_sun_xyz, epsilon, JD );
    venus.position = orbit_venus_xyz;

    // --- Marte ---
    Mars & mars = data->system->mars;
    OrbitXYZ orbit_mars_xyz = mars.orbit_eq( MainBody::mars_data, orbit_sun_xyz, epsilon, JD );
    mars.position = orbit_mars_xyz;

    // --- Giove ---
    Jupiter & jupiter = data->system->jupiter;
    OrbitXYZ orbit_jupiter_xyz = jupiter.orbit_eq( MainBody::jupiter_data, orbit_sun_xyz, epsilon, JD );
    jupiter.position = orbit_jupiter_xyz;

    // --- Saturno ---
    Saturn & saturn = data->system->saturn;
    OrbitXYZ orbit_saturn_xyz = saturn.orbit_eq( MainBody::saturn_data, orbit_sun_xyz, epsilon, JD );
    saturn.position = orbit_saturn_xyz;

    // --- Urano ---
    Uranus & uranus = data->system->uranus;
    OrbitXYZ orbit_uranus_xyz = uranus.orbit_eq( MainBody::uranus_data, orbit_sun_xyz, epsilon, JD );
    uranus.position = orbit_uranus_xyz;

    // --- Nettuno ---
    Neptune & neptune = data->system->neptune;
    OrbitXYZ orbit_neptune_xyz = neptune.orbit_eq( MainBody::neptune_data, orbit_sun_xyz, epsilon, JD );
    neptune.position = orbit_neptune_xyz;

    // --------------------------------------------------------
    // 8. Orizzonte locale per calcolo altezza / azimut
    // --------------------------------------------------------
    Horizont horizont( lat, lon + rot );

    // --------------------------------------------------------
    // 9. SkyGrid per costellazioni e stelle
    // --------------------------------------------------------
    SkyGrid & skyGrid = *data->skyGrid;

    // AR del Sole normalizzato [0,360) — serve per il calcolo dell'elongazione di tutti i corpi
    double sun_ar = o_sun.alfa;
    if ( sun_ar < 0 ) sun_ar += 360.0;

    // --------------------------------------------------------
    // 10. Lambda helper: costruisce il blocco JSON di un corpo
    //     celeste a partire dalle sue coordinate equatoriali
    //     (OrbitEqu contiene alfa=AR in gradi, delta=Dec in gradi, R in AU)
    //
    //     Elongazione = differenza angolare AR corpo - AR sole,
    //     normalizzata in [-180, +180]: positiva = est del sole (serale),
    //     negativa = ovest del sole (mattutina).
    // --------------------------------------------------------
    auto buildBodyJson = [&]( const std::string & name,
                               OrbitEqu         & o_eq,
                               OrbitXYZ         & o_xyz,
                               bool               is_moon ) -> std::string
    {
        double ar  = o_eq.alfa;   // gradi
        double dec = o_eq.delta;  // gradi
        double R   = o_eq.R;      // AU (o unità interna per la luna)

        // AR normalizzato [0,360)
        if ( ar < 0 ) ar += 360.0;

        // AR in ore (0..24)
        double ar_h = 24.0 * ar / 360.0;

        // Altezza e azimut locali rispetto al punto di vista dell'osservatore
        double az_body  = horizont.azimut( Point( o_xyz.x, o_xyz.y, o_xyz.z ) );
        double alt_body = horizont.height( Point( o_xyz.x, o_xyz.y, o_xyz.z ) );

        // Elongazione rispetto al Sole: differenza di AR normalizzata [-180, +180]
        double elongazione = ar - sun_ar;
        if ( elongazione >  180.0 ) elongazione -= 360.0;
        if ( elongazione < -180.0 ) elongazione += 360.0;

        // Distanza
        double distanza_km = 0.0;
        double distanza_au = 0.0;
        if ( is_moon ) {
            // orbit_moon.R è in AU (valore tipico ~0.00257 AU = ~384400 km)
            distanza_km = R * CelestialBody::AU;   // conversione in km
        } else {
            distanza_au = R;
            distanza_km = R * CelestialBody::AU;   // utile per info
        }

        // Costellazione
        std::string constellation = skyGrid.whichConstellation( ar, dec );
        std::string zodiac        = skyGrid.whichZodiac( ar );

        // Stelle principali visibili in quell'area del cielo (±1h ±1° intorno alla posizione)
        std::string stars_raw;
        stars_raw += skyGrid.find( ar_h,     dec     );
        stars_raw += skyGrid.find( ar_h + 1, dec     );
        stars_raw += skyGrid.find( ar_h - 1, dec     );
        stars_raw += skyGrid.find( ar_h,     dec + 1 );
        stars_raw += skyGrid.find( ar_h,     dec - 1 );

        // Costruisce la lista stelle come array JSON di stringhe
        // skyGrid.find() restituisce righe separate da \n
        std::stringstream stars_arr;
        stars_arr << "[";
        {
            std::istringstream iss( stars_raw );
            std::string line;
            bool first = true;
            while ( std::getline( iss, line ) ) {
                if ( line.empty() ) continue;
                // Escape delle virgolette interne
                std::string escaped;
                for ( char c : line ) {
                    if ( c == '"' ) escaped += "\\\"";
                    else            escaped += c;
                }
                if ( !first ) stars_arr << ",";
                stars_arr << "\"" << escaped << "\"";
                first = false;
            }
        }
        stars_arr << "]";

        // Stelle della costellazione (lista nomi)
        std::vector<std::string> const_stars_vec = skyGrid.constellationStars( skyGrid.constellationCodeFromName( constellation ) );
        std::stringstream const_stars_arr;
        const_stars_arr << "[";
        for ( size_t i = 0; i < const_stars_vec.size(); ++i ) {
            std::string s = const_stars_vec[i];
            // Teniamo solo la prima riga (nome)
            size_t nl = s.find('\n');
            if ( nl != std::string::npos ) s = s.substr(0, nl);
            // Rimuove spazi finali
            while ( !s.empty() && ( s.back() == ' ' || s.back() == '\r' ) ) s.pop_back();
            // Escape
            std::string escaped;
            for ( char c : s ) { if ( c == '"' ) escaped += "\\\""; else escaped += c; }
            if ( i > 0 ) const_stars_arr << ",";
            const_stars_arr << "\"" << escaped << "\"";
        }
        const_stars_arr << "]";

        std::stringstream j;
        j.precision(8);
        j << "{"
          <<   "\"name\":\"" << name << "\","
          <<   "\"ra_deg\":"  << ar  << ","
          <<   "\"ra_hms\":\""  << Grade(o_eq.alfa).toGradeHMS() << "\","
          <<   "\"dec_deg\":" << dec << ","
          <<   "\"dec_dms\":\""  << Grade(dec).toGradeGPS() << "\","
          <<   "\"altitude_deg\":" << alt_body << ","
          <<   "\"azimuth_deg\":"  << az_body  << ","
          <<   "\"elongation_deg\":" << elongazione << ","
          <<   "\"constellation\":\"" << constellation << "\","
          <<   "\"zodiac\":\"" << zodiac << "\",";

        if ( is_moon ) {
            j << "\"distance_km\":"  << distanza_km << ",";
        } else {
            j << "\"distance_au\":"  << distanza_au << ","
              << "\"distance_km\":"  << distanza_km << ",";
        }

        j <<   "\"visible\":"  << ( alt_body > 0 ? "true" : "false" ) << ","
          <<   "\"nearby_stars\":" << stars_arr.str() << ","
          <<   "\"constellation_stars\":" << const_stars_arr.str()
          << "}";

        return j.str();
    };

    // --------------------------------------------------------
    // 11. Costruisce il JSON finale
    // --------------------------------------------------------
    std::stringstream result;
    result.precision(8);

    // Data JD corrente
    std::string date_str = d.toDate( JD );

    // Costellazioni e stelle nella direzione di osservazione (punto azimut/alt scelto)
    // Ricaviamo AR/Dec del punto di osservazione dalla camera lookat
    set_horizont( data );   // aggiorna camera.lookat in base a direzione impostata
    clientData::Camera & camera = data->camera;
    OrbitXYZ lookat_xyz( camera.lookat_x, camera.lookat_y, camera.lookat_z );
    OrbitEqu  o_lookat = lookat_xyz;
    double lookat_ar  = o_lookat.alfa;
    double lookat_dec = o_lookat.delta;
    if ( lookat_ar < 0 ) lookat_ar += 360.0;
    double lookat_ar_h = 24.0 * lookat_ar / 360.0;

    std::string sky_stars_raw;
    sky_stars_raw += skyGrid.find( lookat_ar_h,     lookat_dec     );
    sky_stars_raw += skyGrid.find( lookat_ar_h + 1, lookat_dec     );
    sky_stars_raw += skyGrid.find( lookat_ar_h - 1, lookat_dec     );
    sky_stars_raw += skyGrid.find( lookat_ar_h,     lookat_dec + 1 );
    sky_stars_raw += skyGrid.find( lookat_ar_h,     lookat_dec - 1 );

    std::string sky_const = skyGrid.whichConstellation( lookat_ar, lookat_dec );
    std::string sky_zodiac = skyGrid.whichZodiac( lookat_ar );

    // Stelle vicine al punto osservato (come array JSON)
    std::stringstream sky_stars_arr;
    sky_stars_arr << "[";
    {
        std::istringstream iss( sky_stars_raw );
        std::string line;
        bool first = true;
        while ( std::getline( iss, line ) ) {
            if ( line.empty() ) continue;
            std::string escaped;
            for ( char c : line ) { if ( c == '"' ) escaped += "\\\""; else escaped += c; }
            if ( !first ) sky_stars_arr << ",";
            sky_stars_arr << "\"" << escaped << "\"";
            first = false;
        }
    }
    sky_stars_arr << "]";

    // --- Luna: OrbitEqu e OrbitXYZ ---
    OrbitEqu o_moon_eq = moon.position;  // OrbitXYZ → OrbitEqu (conversione implicita)
    OrbitXYZ o_moon_xyz2 = moon.position;

    // --- Sole ---
    OrbitEqu o_sun_eq = sun.position;
    OrbitXYZ o_sun_xyz2 = sun.position;

    // --- Pianeti (conversione ImplicitXYZ→Equ) ---
    OrbitEqu o_mercury_eq = mercury.position;
    OrbitXYZ o_mercury_xyz = mercury.position;

    OrbitEqu o_venus_eq = venus.position;
    OrbitXYZ o_venus_xyz = venus.position;

    OrbitEqu o_mars_eq = mars.position;
    OrbitXYZ o_mars_xyz = mars.position;

    OrbitEqu o_jupiter_eq = jupiter.position;
    OrbitXYZ o_jupiter_xyz = jupiter.position;

    OrbitEqu o_saturn_eq = saturn.position;
    OrbitXYZ o_saturn_xyz = saturn.position;

    OrbitEqu o_uranus_eq = uranus.position;
    OrbitXYZ o_uranus_xyz = uranus.position;

    OrbitEqu o_neptune_eq = neptune.position;
    OrbitXYZ o_neptune_xyz = neptune.position;

    // --------------------------------------------------------
    // 12. Assembla il JSON di output
    // --------------------------------------------------------
    result << "{"
           <<   "\"date\":\"" << date_str << "\","
           <<   "\"julian_day\":" << JD << ","
           <<   "\"observer\":{"
           <<     "\"latitude\":"   << lat        << ","
           <<     "\"longitude\":"  << lon        << ","
           <<     "\"azimut_deg\":" << cam_azimut << ","
           <<     "\"height_deg\":" << cam_height << ","
           <<     "\"direction\":\"" << direction_label << "\""
           <<   "},"
           <<   "\"view_direction\":{"
           <<     "\"ra_deg\":"    << lookat_ar  << ","
           <<     "\"dec_deg\":"   << lookat_dec << ","
           <<     "\"constellation\":\"" << sky_const   << "\","
           <<     "\"zodiac\":\""        << sky_zodiac  << "\","
           <<     "\"nearby_stars\":"    << sky_stars_arr.str()
           <<   "},"
           <<   "\"bodies\":["
           <<     buildBodyJson( "luna",    o_moon_eq,    o_moon_xyz2,  true  ) << ","
           <<     buildBodyJson( "sole",    o_sun_eq,     o_sun_xyz2,   false ) << ","
           <<     buildBodyJson( "mercurio",o_mercury_eq, o_mercury_xyz,false ) << ","
           <<     buildBodyJson( "venere",  o_venus_eq,   o_venus_xyz,  false ) << ","
           <<     buildBodyJson( "marte",   o_mars_eq,    o_mars_xyz,   false ) << ","
           <<     buildBodyJson( "giove",   o_jupiter_eq, o_jupiter_xyz,false ) << ","
           <<     buildBodyJson( "saturno", o_saturn_eq,  o_saturn_xyz, false ) << ","
           <<     buildBodyJson( "urano",   o_uranus_eq,  o_uranus_xyz, false ) << ","
           <<     buildBodyJson( "nettuno", o_neptune_eq, o_neptune_xyz,false )
           <<   "]"
           << "}";

    // --------------------------------------------------------
    // 13. Salva su file e restituisce la stringa JSON
    // --------------------------------------------------------
    {
        // Costruisce un nome file univoco per cliente e data
        std::string fname = "celestial_" + std::to_string(sockfd) + "_" + date_str + ".json";
        // Sostituisce spazi e ':' con underscore per sicurezza sul filesystem
        for ( auto & c : fname )
            if ( c == ' ' || c == ':' ) c = '_';

        std::ofstream ofs( fname );
        if ( ofs.is_open() ) {
            ofs << result.str();
            ofs.close();
        }
    }

    return result.str();
}

std::string WSsrv::handleClient (  int sockfd, const std::string &buffer )
{

    std::string azione ;
    std::string command ;
    try {
      XMLDocument doc;
      doc.parse(buffer);
      if ( buffer.size() != 0 && buffer.at(0) == '<' ) {

        XMLElement * root = doc.rootElement ()  ;

        azione = root->getAttribute ("azione");
        command = root->getAttribute ("data");
      }else {
        azione = "command";
        command = buffer;
      }
    } catch (const std::string& e) {
      std::cout << "Errore di parsing in isXML: " << e << std::endl;
      azione = "command";
      command = buffer;
    } catch (...) {
      std::cout << "Errore sconosciuto isXML." << std::endl;
      azione = "command";
      command = buffer;
    }


#ifdef DEBUG
{
		std::stringstream cc ;
		cc << "handle request " << buffer;

		if ( azione != "reset_timer" )
			print_log( cc.str() );

}
#endif

	// cerca nei clients
	auto it =  clients.find( sockfd ) ;
	if ( it == clients.end() ) return "";

	Client *  client  	= it->second;
	clientData * data = reinterpret_cast < clientData * > ( client->extra );

	if ( !data ) {
    print_log("non ci sono dati");
    return "";
  }

	short simulation = data->simulation;
	Date &d = data->ref_date;
	UT uut = d;
	DT ddt = uut;
	double JD = uut.now( simulation );

//	SolarSystem * system = data->system;

	if ( azione == "command" ) {

		std::string result;

		std::stringstream cc ;
		cc.precision(16);

		Date o;
		cc << command << " ";

		std::string action ;
		cc >> action;

		std::stringstream webgl ;

		if ( action == "mouse" || action == "mouse_move" || action == "mouse_dblclick" ) {

//			double JD = d.now( simulation );

			double latitude = data->lookfrom.latitude;
			double longitude = data->lookfrom.longitude;

			double rot = data->rotation ;
			Earth & earth = data->system->earth ;
			Point coord = earth.bodyCoord ( latitude, longitude + rot );

			double xc = coord.x / CelestialBody::AU;
			double yc = coord.y / CelestialBody::AU;
			double zc = coord.z / CelestialBody::AU;


			Horizont horizont ( latitude, longitude + rot );


			// retta ortogonale al punto di vista
			StraightLine pdv ( Point( 0, 0, 0 ), Point( xc, yc, zc ) );
			// intersezione piano tangente a coord e asse z
			double z_int = ( xc * xc + yc * yc + zc * zc ) / zc;

			// linea in direzione nord
			StraightLine north_line ( Point( xc, yc, zc ), Point( 0, 0, z_int ) );

			// piano tangente a posizione su earth
			Plane tg_plane ( pdv.D, Point( xc, yc, zc ) );

			// posizione sole
			Sun sun;
			OrbitEcli orbit_sun = sun.orbitLBR( JD );	// coordinate eclittiche


			OrbitEqu   o_sun ( JD );
			o_sun = orbit_sun;	// converte in coordinate equatoriali
//			double ar = o_sun.alfa  ;
//			double dec = o_sun.delta;

			std::string type;
			std::string name;
			short button ;
			double x, y, z, distance ;

			cc  >> button >> name >> x >> y >> z >> distance >> type;

			std::stringstream term ;
			term << "<terminal>" << type << " " << name << std::endl ;
			std::stringstream dlg ;
			dlg << "<dialog body=\"" << name << "\">" ;
			std::stringstream dt ;
			dt.precision( 4 );
			dt << "<data>" ;
			
			if ( name == "earth" ) {

				double r_xy = sqrt ( pow( x, 2 ) + pow( y, 2 ) );
				Grade lat = Radiant( atan( z / r_xy ) );



				// determina il quadrante
				bool sign_y = fabs(y) / y > 0 ? true : false;
				bool sign_x = fabs(x) / x > 0 ? true : false;
				short quadrante = 0;

				if ( sign_x && sign_y  ) quadrante = 1;
				else if ( !sign_x && sign_y ) quadrante = 2;
				else if ( !sign_x && !sign_y ) quadrante = 3;
				else if ( sign_x && !sign_y ) quadrante = 4;

				Grade lng  = Radiant( atan( fabs(y) / fabs(x) ) );

				switch ( quadrante ) {
					case 2 : lng = 180 - lng ;
					break;
					case 3 : lng = 180 + lng ;
					break;
					case 4 : lng = 360 - lng ;
					break;
					default :
					break;
				}


				lng = lng - data->rotation;
				std::stringstream tip  ;


if ( button == 2 ) {

				try {


//http://api.geonames.org/cities?north=44.1&south=-9.9&east=-22.4&west=55.2&username=demo
					double north = lat + 1;
					double south = lat - 1;
					double east = lng + 1;
					double west = lng - 1;

					std::stringstream query ;
					query << "north=" << north
						<< "&south=" << south
						<< "&east=" << east
						<< "&west=" << west
						<< "&username=baldoweb" ;

					TCPClient geoLoc ( "api.geonames.org" ) ;
					std::string geo_result = geoLoc.get( "/cities", query.str() );
//					print_log( geo_result );

					XMLDocument doc;
					doc.parse( geo_result );
					XMLElement * root = doc.rootElement ()  ;

					auto geoname =  root->getElementsByTagName ( "geoname" ) ;
					for ( size_t i = 0; i < geoname.size(); ++i ) {

						XMLElement *geo = geoname.at(i);
						XMLElement * e_name =  geo->getFirstElementByTagName ( "name" ) ;
						XMLElement * e_countryName = geo->getFirstElementByTagName ( "countryName" ) ;
						XMLElement * e_lat = geo->getFirstElementByTagName ( "lat" ) ;
						XMLElement * e_lng = geo->getFirstElementByTagName ( "lng" ) ;

						std::string countryName, name, lat, lng;
						if ( e_countryName )
							countryName = e_countryName->innerHTML();
						if ( e_name  )
							name = e_name->innerHTML();
						if ( e_lat  )
							lat = e_lat->innerHTML();
						if ( e_lng  )
							lng = e_lng->innerHTML();

						dlg << "<form style=\"text-align:center;margin:1px\""
								<< " onsubmit=\""
								<< "exe_command ( 'zoom 1' );"
								<< "exe_command ( 'observe " << lng << " " << lat << " zenit' );"
								<< "return false;\">"
								<< "<input type=\"submit\" value=\"" << name
								<< ", " << countryName
								<< "\" /></form>"  ;


					}

				} catch ( const char * str ) {
					print_log( str );
				} catch ( const std::string & str ) {
					print_log( str );
				}
				catch ( ... ) {
					print_log( "***" );
				}

}

				dlg << "<p>Latitudine = " << lat  << "</p><p>Longitudine = " << lng << "</p>"  ;
				term << "x=" << x << " y=" << y << " z=" << z << std::endl;
				term << "Latitudine = " << lat << " Longitudine = " << lng << std::endl;


				if ( tip.str() == "" ) {
					tip << "Long " << lng << " Lat " << lat   ;
					dt << "<div id=\"tip1\" value=\"" << name << "\" />";
					dt << "<div id=\"tip\" value=\"" << tip.str() << "\" />";
				}
				else {
					dt << "<div id=\"tip1\" value=\"" << name << "\" />";
					dt << "<div id=\"tip\" value=\"" << tip.str() << "\" />";
				}

				if ( action == "mouse_dblclick" ) {


					data->lookfrom.isFromPlace = true;
					data->lookfrom.latitude = lat;
					data->lookfrom.longitude = lng;
					data->lookfrom.where = "earth";

					data->lookat.where = "zenit";

				}

			}
			else if ( name == "sky" ) {



				std::stringstream hor_tip;
				std::stringstream hor;
				if ( data->lookfrom.where == "earth" ) {


					double latitude = data->lookfrom.latitude;
					double longitude = data->lookfrom.longitude;

					Horizont horizont ( latitude, longitude + rot );

					double azimut = horizont.azimut ( Point( x, y, z )  ) ;
					double height = horizont.height ( Point( x, y, z )  ) ;



					if ( type == "down" ) {

						data->mouse.pressed = true;
						data->mouse.altezza = height  ;
						data->mouse.azimut = azimut  ;


					}
					else  if ( type == "up" ) {

						if ( fabs( height - data->mouse.altezza ) > 1 ) {
							data->lookat.altezza  -= 1 * fabs( height - data->mouse.altezza ) / ( height - data->mouse.altezza );
						}

						if ( fabs( azimut - data->mouse.azimut )> 1  ) {
							data->lookat.azimut -= 1* fabs( azimut - data->mouse.azimut ) / ( azimut - data->mouse.azimut );
						}
						data->mouse.pressed = false;
						data->lookat.view = clientData::lookAt::VIEW::AZIMUT;

					}
					else  if ( type == "move" ) {
/*
						if ( data->mouse.pressed ) {
							data->lookat.altezza  -= ( height - data->mouse.altezza );
							data->lookat.azimut -= ( azimut - data->mouse.azimut );
							data->lookat.view = clientData::lookAt::VIEW::AZIMUT;
						}
*/
					}

					hor.precision(4);
					hor <<
						"<p>Altezza sull'orizzonte (gradi) " << height << "</p>" 	<<
						"<p>Azimut (gradi) " << azimut << "</p>" 	;

					hor_tip.precision(4);
					hor_tip <<
						" Alt " << height << " " 	<<
						"Azi " << azimut  	;

				}

				// lookto
				StraightLine lookat ( Point( xc, yc, zc ), Point( x, y, z ) );
				Radiant angle = pdv.angle ( lookat ) ;

				OrbitXYZ o_xyz ( x, y, z ) ;
				OrbitEqu o_eq = o_xyz ;

				float Dec = o_eq.delta ;
				float AR = o_eq.alfa ;	// gradi

				float ar = AR;

				double elongazione = ar - o_sun.alfa;

 				if ( AR < 0 )
					AR = 360 + AR  ;

				SkyGrid & skyGrid = * data->skyGrid;
				std::string const_name = skyGrid.whichConstellation( AR, Dec );
				std::string const_code = skyGrid.constellationCodeFromName( const_name );
				std::stringstream zodiaco ;


				if ( fabs(Dec) < 15 ) {

					zodiaco <<" [zodiac:" << skyGrid.whichZodiac ( AR ) << "]";

				}

				std::stringstream tip ;

 				AR = 24 * AR / 360;
				tip << skyGrid.find( AR, Dec );
				tip << skyGrid.find( AR+1, Dec );
				tip << skyGrid.find( AR-1, Dec );
				tip << skyGrid.find( AR, Dec+1 );
				tip << skyGrid.find( AR, Dec-1 );

				term <<
					"Dec = " << Dec  << " (" << Grade(Dec).toGradeGPS() << ")" << std::endl <<
						"AR = " << AR << " (" << Grade(ar).toGradeHMS() << ")";

				dlg <<
					"<p>" << tip.str()  << "</p>" <<
					"<p>" << hor.str()  << "</p>" <<
					"<p>Elongazione (gradi) " << elongazione << "</p>" 	<<
					"<p>Dec = " << Dec  << " (" << Grade(Dec).toGradeGPS() << ")</p>" <<
					"<p>AR = " << AR << " (" << Grade(ar).toGradeHMS() << ")</p>" 	<<

					"" 	;

				if ( const_code != "" ) {

					// verifica se gli asterismi sono attivi
					auto it = data->the_constellation.find( const_code );
					std::string asterism_checked, tracciato_checked ;
					if (it == data->the_constellation.end()) {
						asterism_checked = "";
						tracciato_checked = "";
					}
					else {
						asterism_checked = it->second.asterism ? "checked=\"checked\"" : "";
						tracciato_checked = it->second.tracciato ? "checked=\"checked\"" : "";
					}

					dlg <<
						"<div class=\"ui-body ui-body-a ui-corner-all\">" <<
						"<h3>" <<	const_name << " [" << const_code << "]" << "</h3>" <<
						"<p>" <<
						"<label><input " << tracciato_checked << " class=\"costellazione\" data-costellazione=\"" << const_code << "\" type=\"checkbox\" />Constellation</label>" <<
						"</p>" <<

						"<p>" <<
						"<label><input " << asterism_checked << " class=\"asterismo\" data-costellazione=\"" << const_code << "\" type=\"checkbox\" />" << "Asterism" << "</label>" <<
						"</p>" <<


						"</div>" <<
						"" 	;


					std::vector< std::string> stars = skyGrid.constellationStars (const_code ) ;

					std::stringstream select_stars;

					std::vector<std::string>::iterator it_stars;

					select_stars << "<h3 class=\"ui-bar ui-bar-a\">Stars/Messiers</h3><select class=\"ui-body constellation_star\">><option></option>";
					for( it_stars = stars.begin(); it_stars != stars.end(); it_stars++ )    {
						std::string str = *it_stars;
						int  p = str.find("\n");
						std::string content = str.substr(0,p-1);
						std::string value = str.substr ( p+1);

						select_stars << "<option  value=\"" << value << "\">" << content << "</option>";

					}
					select_stars << "</select>";

					dlg << select_stars.str() ;

				}




				if ( tip.str() != "" ) {
					dt << "<div id=\"tip1\" value=\"" << tip.str() << "\" />";
					dt << "<div id=\"tip\" value=\"AR=" <<
						AR << " Dec=" << Dec << hor_tip.str() << "\" />";
				}
				else {

					SkyGrid::Grid grid ;
					grid = SkyGrid::SkyCoord ( AR, Dec );

//					short quadrante = grid ;

					dt << "<div id=\"tip1\" value=\"" << "sky " << const_name <<
						zodiaco.str() <<  "\" />";
					dt << "<div id=\"tip\" value=\"AR=" <<
						AR << " Dec=" << Dec << hor_tip.str() << "\" />";
				}

				if ( action == "mouse_dblclick" ) {


					if ( data->lookfrom.where == "earth" ) {


						clientData::Camera &camera = data->camera;
						camera.lookat_x = x ;
						camera.lookat_y = y ;
						camera.lookat_z = z ;

						data->lookat.view = clientData::lookAt::VIEW::NONE;

					}
					else {

						data->lookfrom.isFromPlace = true;
						data->lookfrom.latitude = Dec;
						data->lookfrom.longitude = ar;

						data->lookat.where = "zenit";


					}

				}


			}
			else if ( name == "sun" ) {


				double ar = o_sun.alfa  ;
				double dec = o_sun.delta;

 				if ( ar < 0 )
					ar = 360 + ar  ;

				SkyGrid & skyGrid = * data->skyGrid;
				std::string constellation = skyGrid.whichConstellation ( ar, dec ) ;
				std::string zodiaco = skyGrid.whichZodiac ( ar ) ;

				Grade L = orbit_sun.L ;
				Grade B = orbit_sun.B ;
				Grade alfa = o_sun.alfa;
				Grade delta = o_sun.delta; // declinazione

				OrbitXYZ xyz_sun = o_sun ;	// converte da coordinate equatoriali in rettangolari

				double x_sun = xyz_sun.x ;
				double y_sun = xyz_sun.y ;
				double z_sun = xyz_sun.z ;

				double azimut = horizont.azimut ( Point( x_sun, y_sun, z_sun )  ) ;
				double height = horizont.height ( Point(  x_sun, y_sun, z_sun  )  ) ;

				// calcola direttamente con vsop le coordinate rettangolari
				OrbitXYZ orbit_sun_xyz = sun.orbit ( MainBody::earth_data, JD ) ;

				double x_s = -orbit_sun_xyz.x  ;
				double y_s = -orbit_sun_xyz.y ;
				double z_s = -orbit_sun_xyz.z  ;

				Earth earth;
				double epsilon = earth.obliquity ( JD );
				Radiant eps = Grade( epsilon );

				double x_eq = x_s;
				double y_eq = y_s * cos( eps ) - z_s * sin( eps );
				double z_eq = y_s * sin( eps ) + z_s * cos( eps );


				term <<
					"x=" << x_sun << " y=" << y_sun <<  " z=" << z_sun << std::endl <<
					"x=" << x_eq << " y=" << y_eq <<  " z=" << z_eq << std::endl <<
					"x=" << x_s << " y=" << y_s <<  " z=" << z_s << std::endl <<
					"L = " <<  L <<" (" << L.toGradeGPS() << ")" << std::endl <<
					"B = " << B << " (" << B.toGradeGPS() << ")" << std::endl <<
					"AR = " << alfa << " (" << alfa.toGradeHMS() << ")" << std::endl <<
					"Dec = " <<  delta << " (" << delta.toGradeGPS() << ")" << std::endl <<
					"R = " << orbit_sun.R  ;

				dlg <<
					"<p>Altezza sull'orizzonte (gradi) " << height << "</p>" 	<<
					"<p>Azimut (gradi) " << azimut << "</p>" 	<<
					"<p>AR = " << alfa << " (" << alfa.toGradeHMS() << ")</p>" <<
					"<p>Dec = " <<  delta << " (" << delta.toGradeGPS() << ")</p>" <<
					"<p>R = " << orbit_sun.R  << "</p>"
				;
				dt << "<div id=\"tip1\" value=\"" << name << " in " <<
					constellation << " [zodiac: " << zodiaco << "]\" />";
				dt << "<div id=\"tip\" value=\"\" />";

			}
			else if ( name == "moon" ) {


				Moon moon;

				OrbitEcli orbit_moon = moon.orbitAlfaELP( JD );
				Grade lambda = orbit_moon.L;
				Grade beta = orbit_moon.B;

				OrbitEqu   o_moon ( JD );
				o_moon = orbit_moon;

				double ar = o_moon.alfa  ;
				double dec = o_moon.delta;

				double elongazione = ar - o_sun.alfa;

 				if ( ar < 0 )
					ar = 360 + ar  ;

				SkyGrid & skyGrid = * data->skyGrid;
				std::string constellation = skyGrid.whichConstellation ( ar, dec ) ;


				OrbitXYZ o = o_moon;
				double x_moon = o.x;
				double y_moon = o.y;
				double z_moon = o.z;


				double azimut = horizont.azimut ( Point( x_moon, y_moon, z_moon )  ) ;
				double height = horizont.height ( Point(  x_moon, y_moon, z_moon  )  ) ;


				// traslazione
				x -= x_moon;
				y -= y_moon;
				z -= z_moon;

				double r_xy = sqrt ( pow( x, 2 ) + pow( y, 2 ) );
				Grade lat = Radiant( atan( z / r_xy ) );



				// determina il quadrante
				bool sign_y = fabs(y) / y > 0 ? true : false;
				bool sign_x = fabs(x) / x > 0 ? true : false;
				short quadrante = 0;

				if ( sign_x && sign_y  ) quadrante = 1;
				else if ( !sign_x && sign_y ) quadrante = 2;
				else if ( !sign_x && !sign_y ) quadrante = 3;
				else if ( sign_x && !sign_y ) quadrante = 4;

				Grade lng  = Radiant( atan( fabs(y) / fabs(x) ) );

				switch ( quadrante ) {
					case 2 : lng = 180 - lng ;
					break;
					case 3 : lng = 180 + lng ;
					break;
					case 4 : lng = 360 - lng ;
					break;
					default :
					break;
				}


				Grade rot_luna =  180 + o_moon.alfa;

				lng = lng - rot_luna;

				std::stringstream tip = moonGrid->object( lng, lat ) ;

/*
				if ( data->camera.zoom < 2 ) {
					dt << "<div id=\"tip1\" value=\"" << name << " in " << constellation << "\" />";
					dt << "<div id=\"tip\" value=\"\" />";
				}
*/
				if ( tip.str() == "" ) {
					tip << " Long " << lng << " Lat " << lat   ;
					dt << "<div id=\"tip1\" value=\"" << name << "\" />";
					dt << "<div id=\"tip\" value=\"" << tip.str() << "\" />";
				}
				else {
					dt << "<div id=\"tip1\" value=\"" << name << "\" />";
					dt << "<div id=\"tip\" value=\"" << tip.str() << "\" />";
				}

				term <<
					"AR = " << o_moon.alfa << " (" << o_moon.alfa.toGradeHMS() << ")" << std::endl <<
					"Dec = " <<  o_moon.delta << " (" << o_moon.delta.toGradeGPS() << ")" << std::endl <<
					"lambda = " <<  lambda << " (" << lambda.toGradeGPS() << ")" << std::endl <<
					"beta = " << beta << " (" << beta.toGradeGPS() << ")" << std::endl <<
					"R = " << orbit_moon.R * CelestialBody::AU   << " km" << std::endl ;
				term << "Latitudine = " << lat << " Longitudine = " << lng ;

				dlg <<
					name << " " << tip.str() <<
					"<p>Altezza sull'orizzonte (gradi) " << height << "</p>" 	<<
					"<p>Azimut (gradi) " << azimut << "</p>" 	<<
					"<p>Elongazione (gradi) " << elongazione << "</p>" 	<<
					"<p>Latitudine = " << lat << " Longitudine = " << lng << "</p>" <<
					"<p>AR = " << o_moon.alfa << " (" << o_moon.alfa.toGradeHMS() << ")</p>" <<
					"<p>Dec = " <<  o_moon.delta << " (" << o_moon.delta.toGradeGPS() << ")</p>" <<
					"<p>R = " << orbit_moon.R * CelestialBody::AU   << " km" << "</p>"
				;

			}
			else if (
				name == "mercury" ||
				name == "venus" ||
				name == "mars" ||
				name == "jupiter" ||
				name == "saturn" ||
				name == "uranus"
				) {

				const VSOP * vsop ;
				if ( name == "venus" )
					vsop = & MainBody::venus_data;
				else if ( name == "mars" )
					vsop = & MainBody::mars_data;
				else if ( name == "jupiter" )
					vsop = & MainBody::jupiter_data;
				else if ( name == "mercury" )
					vsop = & MainBody::mercury_data;
				else if ( name == "saturn" )
					vsop = & MainBody::saturn_data;
				else if ( name == "uranus" )
					vsop = & MainBody::uranus_data;

				MainBody celestialBody;
				OrbitXYZ o_v = celestialBody.orbit_eq ( * vsop, JD );

				double x = o_v.x ;
				double y = o_v.y ;
				double z = o_v.z ;



				double azimut = horizont.azimut ( Point( x, y, z )  ) ;
				double height = horizont.height ( Point(  x, y, z  )  ) ;


				OrbitXYZ o_xyz ( x, y, z ) ;
				OrbitEqu o_eq = o_xyz ;

				Grade Dec = o_eq.delta ;
				Grade AR = o_eq.alfa ;

				double ar = o_eq.alfa  ;
				double dec = o_eq.delta;

 				if ( ar < 0 )
					ar = 360 + ar  ;

				SkyGrid & skyGrid = * data->skyGrid;
				std::string constellation = skyGrid.whichConstellation ( ar, dec ) ;


				term <<
					name << " "	<< x << " " << y << " " << z << std::endl <<
					"Dec = " << Dec  << " (" << Dec.toGradeGPS() << ")" << std::endl <<
						"AR = " << AR << " (" << AR.toGradeHMS() << ")";

				dlg <<
					"<p>Altezza sull'orizzonte (gradi) " << height << "</p>" 	<<
					"<p>Azimut (gradi) " << azimut << "</p>" 	<<
					"<p>Dec = " << Dec  << " (" << Dec.toGradeGPS() << ")</p>" <<
					"<p>AR = " << AR << " (" << AR.toGradeHMS() << ")</p>" 	<<
//					"<p>"	<< x << " " << y << " " << z << "</p>" <<
					"<p>"	<< "R=" <<  o_eq.R << "</p>" ;
				dt << "<div id=\"tip1\" value=\"" << name << " in " << constellation << "\" />";
				dt << "<div id=\"tip\" value=\"\" />";

			}
			else {
				term <<
					name << " at  "	<< x << " " << y << " " << z << std::endl ;
			}

			term <<  "</terminal>" ;
			dlg << "</dialog>" ;
			dt << "</data>" ;


			std::stringstream ss ;
			ss.precision( 12 );
			ss << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" ;
			ss << "<results>" ;

			if ( action == "mouse" ) {

				ss << term.str();
				switch ( button ) {

					case 0 :	// tasto sx
						ss << dlg.str();
					break;
					case 1 :
					break;
					case 2 :	// tasto dx
					break;
				}
			}
			else {
			}


			ss << "</results>" ;

			return ( ss.str() );

		}
		else if ( action == "sun_rise" ) {


			if ( data->lookfrom.isFromPlace ) {

				double lat = data->lookfrom.latitude ;
				double lng = data->lookfrom.longitude ;


				Earth earth;
				double jd = earth.sunRise( d, lat, lng, simulation);
				Date d1 = jd;

				auto it = data->system->bodies.find("sun");

				CelestialBody & body = it->second;
				OrbitEqu o = body.position;
				Grade ar = o.alfa  ;
				Grade dec = o.delta;


				Radiant lt = Grade( lat );
				Radiant dc = dec;
				double cosomega0 =
					( sin( Radiant( Grade(-0.83) ) ) - sin( lt ) * sin( dc ) )
					/
					( cos( lt ) * cos ( dc ) ) ;

				Grade  omega = Radiant ( acos( cosomega0 ) );

				double h = ( Grade( omega ) - lng ) * 24 / 360 ;

				SideralTime st ( d );
				Grade a (st.teta0());


				std::stringstream xml;
				xml.precision(16);
				xml <<
					"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
					"<results>" <<
						"<terminal>" << d1.toDate() << " (" << jd << ") " << h << " " << a.toGradeHMS() << " " << a << "</terminal>" <<
					"</results>"
				;

				return ( xml.str() );

			}

		}
		else if ( action == "angle" ) {


			if ( data->lookfrom.isFromPlace ) {

				double lat = data->lookfrom.latitude ;
				double lng = data->lookfrom.longitude ;

				double JD = d.now( simulation );

				if ( !cc.eof() )
					cc >> JD;

				Earth earth;
				double angle = earth.sunAngleOverOrizont( JD, lat, lng ) ;

				std::stringstream xml;
				xml.precision(8);
				xml <<
					"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
					"<results>" <<
						"<terminal>" << angle  << " at " << JD << "</terminal>" <<
					"</results>"
				;
				return ( xml.str() );

			}

		}
		else if ( action == "rotation" ) {

			double rot;
			cc >> rot;
			double y = tan( rot * 3.14 / 180 );
			double x = tan( (90-rot) * 3.14 / 180 );
			clientData::Camera &camera = data->camera;

			double x2 = camera.lookat_x + x;
			double y2 = camera.lookat_y + y;
			double z2 = camera.lookat_z;

//			double r = sqr( pow(x2-x1,2) + pow(y2-y1,2) )  ;
			camera.up_x = x2 ;
			camera.up_y = y2 ;
			camera.up_z = z2+1 ;

		}
		else if ( action == "move" ) {
			double height, azimut;
			
			// Se riesce a leggere height, applica l'altezza
			if ( cc >> height ) {
				data->lookat.altezza += height ;
				
				// Se riesce a leggere ANCHE l'azimut, applica l'azimut
				if ( cc >> azimut ) {
					data->lookat.azimut += azimut ;
					data->lookat.view = clientData::lookAt::VIEW::AZIMUT;
				}
			}

			std::stringstream xml;

			xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
				"<results>" <<
					"<terminal>" <<
						"Spostamento: altezza " << height <<
						" azimut " << azimut <<
					"</terminal>" <<
				"</results>" ;

			return ( xml.str() );
		}
		else if ( action == "observe" ) {


 			double longitudine, latitudine;
			cc >> longitudine >> latitudine ;

			std::string verso;
			if ( cc.eof() ) {

				if ( data->lookfrom.isFromPlace ) {

					latitudine = data->lookfrom.latitude ;
					longitudine = data->lookfrom.longitude ;

				}

			}
			else {

				if ( cc.eof() ) {
					data->lookat.where = "sun";
				}
				else {
					cc >> verso;

					if ( verso == "N" ) {

						data->lookat.where = "sky";
						data->lookat.view = clientData::lookAt::VIEW::NORTH;

					}
					else if ( verso == "NE" ) {

						data->lookat.where = "sky";
						data->lookat.view = clientData::lookAt::VIEW::NORTH_EAST;

					}
					else if ( verso == "NW" ) {

						data->lookat.where = "sky";
						data->lookat.view = clientData::lookAt::VIEW::NORTH_WEST;

					}
					else if ( verso == "SE" ) {

						data->lookat.where = "sky";
						data->lookat.view = clientData::lookAt::VIEW::SOUTH_EAST;

					}
					else if ( verso == "SW" ) {

						data->lookat.where = "sky";
						data->lookat.view = clientData::lookAt::VIEW::SOUTH_WEST;

					}
					else if ( verso == "S" ) {

						data->lookat.where = "sky";
						data->lookat.view = clientData::lookAt::VIEW::SOUTH;

					}
					else if ( verso == "E" ) {

						data->lookat.where = "sky";
						data->lookat.view = clientData::lookAt::VIEW::EAST;

					}
					else if ( verso == "W" ||  verso == "O") {

						data->lookat.where = "sky";
						data->lookat.view = clientData::lookAt::VIEW::WEST;

					}
					else if ( verso == "Z" ) {

						data->lookat.where = "sky";
						data->lookat.view = clientData::lookAt::VIEW::ZENIT;

					}
					else {
						data->lookat.where = verso;
					}

				}

				data->lookfrom.where = "earth";
				data->lookfrom.isFromPlace = true;
				data->lookfrom.latitude = latitudine;
				data->lookfrom.longitude = longitudine;

			}

			std::stringstream ss ;

			Horizont horizont ( latitudine, longitudine + data->rotation );
			Point pdv = horizont.pointOfView();
			Point center ( 0, 0, 0 );
			double distance = center.distance ( pdv );

			ss << "Latitudine " << latitudine << " Longitudine " << longitudine <<
				" Distanza dal centro " << distance * CelestialBody::AU <<
				" look at " << data->lookat.where << " direzione " << verso;


			std::stringstream xml;


			xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
				"<results>" <<
					"<terminal>" << ss.str() << "</terminal>" <<
				"</results>" ;



			return ( xml.str() );

		}
		else if ( action == "coordinate" ) {

			if ( !cc.eof() ) {

				double latitudine, longitudine;

				cc >> longitudine >> latitudine ;
				data->lookfrom.latitude = latitudine;
				data->lookfrom.longitude = longitudine;

			}

		}
		else if ( action == "lookfrom" ) {

			std::string fromwhere;
			cc >> fromwhere;
			if ( cc.eof() ) {

				return "";

			}

			data->lookfrom.where = fromwhere;

 			double longitudine, latitudine;
			cc >> longitudine >> latitudine ;

			if ( cc.eof() ) {

				latitudine = data->lookfrom.latitude ;
				longitudine = data->lookfrom.longitude ;

			}
			else {

				std::string verso;
				cc >> verso;
				if ( cc.eof() ) {
					data->lookat.where = "sun";
				}
				else {

					data->lookat.where = verso;

				}

				data->lookfrom.latitude = latitudine;
				data->lookfrom.longitude = longitudine;

			}

			data->lookfrom.isFromPlace = true;
			std::stringstream the_data ;
			std::stringstream ss ;
			if ( fromwhere == "earth" ) {

				the_data <<
					"<input id=\"latitudine\" value=\"" << latitudine << "\" />"  <<
					"<input id=\"longitudine\" value=\"" << longitudine << "\" />"  ;

				short y = d.year();
				short m = d.month() + 1;
				short g = d.day();
				std::stringstream query ;
				query << "lat=" << latitudine
						<< "&lng=" << longitudine
						<< "&username=baldoweb"
						<< "&date=" << y << "-" << m << "-" << g ;

				TCPClient geoLoc ( "api.geonames.org" ) ;
				std::string geo_result = geoLoc.get( "/timezone", query.str() );

				ss << "thanks to geonames.org" << std::endl;

				XMLDocument doc;
				doc.parse( geo_result );
				XMLElement * root = doc.rootElement ()  ;

				auto geoname =  root->getElementsByTagName ( "timezone" ) ;
				for ( size_t i = 0; i < geoname.size(); ++i ) {

					XMLElement *geo = geoname.at(i);
					XMLElement * e_timezoneId =  geo->getFirstElementByTagName ( "timezoneId" ) ;
					XMLElement * e_countryName = geo->getFirstElementByTagName ( "countryName" ) ;
					XMLElement * e_lat = geo->getFirstElementByTagName ( "lat" ) ;
					XMLElement * e_lng = geo->getFirstElementByTagName ( "lng" ) ;

					XMLElement * e_gmtOffset = geo->getFirstElementByTagName ( "gmtOffset" ) ;
					XMLElement * e_dstOffset = geo->getFirstElementByTagName ( "dstOffset" ) ;
					XMLElement * e_rawOffset = geo->getFirstElementByTagName ( "rawOffset" ) ;
					XMLElement * e_time = geo->getFirstElementByTagName ( "time" ) ;

					XMLElement * e_sunrise = geo->getFirstElementByTagName ( "sunrise" ) ;
					XMLElement * e_sunset = geo->getFirstElementByTagName ( "sunset" ) ;

					std::string timezoneId, countryName, lat,
						lng, gmtOffset, time, sunrise, sunset ;

					if ( e_countryName )
						ss << "Country " << e_countryName->innerHTML();
					if ( e_timezoneId )
						ss << "Zone " << e_timezoneId->innerHTML();
					if ( e_lat )
						ss << "Lat " << e_lat->innerHTML();
					if ( e_lng )
						ss << "Lng " << e_lng->innerHTML();
					if ( e_gmtOffset ) {
						ss << "gmtOffset " << e_gmtOffset->innerHTML();
					}
					if ( e_dstOffset ) {
						ss << "dstOffset " << e_dstOffset->innerHTML();
					}
					if ( e_rawOffset ) {
						ss << "rawOffset " << e_rawOffset->innerHTML();
						data->lookfrom.dstOffset = atof( e_rawOffset->innerHTML().c_str() );
					}
					if ( e_time )
						ss << "Time " << e_time->innerHTML();
					if ( e_sunrise )
						ss << "Sunrise " << e_sunrise->innerHTML();
					if ( e_sunset )
						ss << "Sunset " << e_sunset->innerHTML();

					ss << std::endl;


				}

			}


			std::stringstream xml;

			xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
				"<results>" <<
					"<terminal>" <<
						"lookfrom " << data->lookfrom.where << " to " << data->lookat.where << std::endl <<
						"lat " << data->lookfrom.latitude << " lng " << data->lookfrom.longitude << std::endl << ss.str() << "</terminal>" <<
					"<data>" << the_data.str() << "</data>" <<
				"</results>" ;



			return ( xml.str() );

		}
		else if ( action == "sky" ) {

			Sky sky;

			double R = sky.radius;
			double ascensione_retta ;
			double declinazione ;

			cc >> ascensione_retta ;

			std::stringstream ss ;

			if ( cc.eof() ) {

				ss << "Please set R.A. (hours) and Dec (degree)" << std::endl;

			}
			else {

				cc >> declinazione;

				if ( cc.eof() ) {
					ss << "Please set R.A. (hours) and Dec (degree)" << std::endl;
				}
				else {

					declinazione += data->lookfrom.move_lat;
					ascensione_retta += data->lookfrom.move_lng;

					Radiant a = Grade( ascensione_retta * 360 / 24  );
					Radiant d = Grade( declinazione  );

					double z = R * sin( d );
					double r = R * cos( d );

					double x = r * cos( a );
					double y = r * sin( a );
					
					// Calcola Altezza e Azimut se l'osservatore è sulla Terra
					if ( data->lookfrom.where == "earth" ) {
						double lat = data->lookfrom.latitude;
						double lon = data->lookfrom.longitude;
						double rot = data->rotation;

						// Inizializza l'orizzonte locale
						Horizont horizont(lat, lon + rot);

						// Usa i metodi nativi della classe Horizont passando il punto 3D
						data->lookat.altezza = horizont.height(Point(x, y, z));
						data->lookat.azimut  = horizont.azimut(Point(x, y, z));
						
						// Opzionale: se la logica della tua applicazione richiede 
						// di impostare la VIEW specifica per far aggiornare il render
						data->lookat.view = clientData::lookAt::VIEW::AZIMUT;
					}
					// -----------------------------

					clientData::Camera &camera = data->camera;
					camera.lookat_x = x ;
					camera.lookat_y = y ;
					camera.lookat_z = z ;

					data->lookat.where = "sky";

					std::string AR = AscensioneRetta( a );
					std::string Dec = Declinazione( d );

					if ( data->lookfrom.where == "earth" ) {
						data->lookat.view = clientData::lookAt::VIEW::NONE;
					}

					ss <<
						"RA=" << AR << " Dec=" << Dec << " x=" << x <<
						" y=" << y <<
						" z=" << z  ;

					std::stringstream xml;
					xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
						"<results>" <<
						"	<renderer>" <<
                        "	    <camera>" <<
                        "		    <lookat x=\"" << x << "\" y=\"" << y << "\" z=\"" << z<< "\" />" <<
                        "	    </camera>" <<

                        // MODIFICA QUI: Abbiamo cambiato il tipo e il colore
                        "       <primitive type=\"target\" name=\"center\">" <<
                        "           <recreate />" <<
                        "           <vertice>" << x << " " << y << " " << z << "</vertice>" <<
                        // Usiamo un Ciano acceso (0x00ffff), size più grande e type "crosshair"
                        "           <material size=\"20\" color=\"0x00ffff\" type=\"crosshair\" />" <<
                        "       </primitive>" <<

				        "   </renderer>" <<
						"</results>" ;

					// manda i dati al client
					std::string type = "text";
					return ( xml.str() );

				}
			}
		}
		else if ( action == "constellation" ) {

			Sky sky;

			std::string name, n1, n2;
			cc >> n1 >> n2;

			name = utils::trim(n1)  ;

			bool hidden = false;
			if ( utils::trim(n2) == "hidden" ) {
				hidden = true;
			}

			// tiene memoria se gli asterismo e i tracciati vanno mostrati
			auto it = data->the_constellation.find( name );
			if (it == data->the_constellation.end()) {
				clientData::Constellation co;
				data->the_constellation[name]=co ;
				data->the_constellation[name].tracciato = !hidden;
			}
			else {
				it->second.tracciato = !hidden;
			}

			data->tracciato = data->skyGrid->drawBounds ( name, hidden );

			std::stringstream xml;
			xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
						"<results>" <<
						"	<renderer>" <<
								data->skyGrid->drawBounds ( name, hidden ) <<
						"	</renderer>"  <<
						"</results>" ;

			// manda i dati al client
			std::string type = "text";
			return ( xml.str() );

		}
		else if ( action == "asterism" ) {

			Sky sky;

			std::string name, n1, n2;
			cc >> n1 >> n2;

			name = utils::trim (n1)  ;

			bool hidden = false;
			if ( utils::trim(n2) == "hidden" ) {
				hidden = true;
			}

			// tiene memoria se gli asterismo e i tracciati vanno mostrati
			auto it = data->the_constellation.find( name );
			if (it == data->the_constellation.end()) {
				clientData::Constellation co;
				data->the_constellation[name]=co ;
				data->the_constellation[name].asterism = !hidden;
			}
			else {
				it->second.asterism = !hidden;
			}
//   mymap.erase (it);


			data->asterismi = "";
      try {
        data->asterismi = data->skyGrid->asterism ( name, hidden );
      }
      catch (const std::runtime_error& e) {
          std::cerr << "Errore runtime: " << e.what() << "\n";
          return "";
      }
      catch (const std::exception& e) {
          // intercetta anche altri errori standard
          std::cerr << "Eccezione generica: " << e.what() << "\n";
          return "";
      }
      catch (...) {
          // intercetta tutto ciò che non è std::exception
          std::cerr << "Errore sconosciuto!\n";
          return "";
      }

			std::stringstream xml;
			xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
						"<results>" <<
							"<renderer>" <<
								data->asterismi <<
							"</renderer>"  <<
						"</results>" ;

			// manda i dati al client
			std::string type = "text";
			return ( xml.str() );

		}
		else if ( action == "date" ) {

			std::stringstream ss ;
			std::string date;
			std::string time;
			cc >> date;

			if ( !cc.eof()  ) {

				cc >> time;
				if ( cc.eof() )
					time ="00:00:00";

				std::string ut;
				cc >> ut;
				if ( cc.eof() )
					ut = "UT";

				std::string dt = date + " " + time;
				if ( ut == "DT" ) {

					DT dtt = dt;
					UT utt = dtt;

					data->ref_date = utt;

				}
				else {

					data->ref_date = dt;

				}


			}
			else {
				ss << "Format dd-mm-yyy hh:mm:ss" << std::endl;
			}

			double jd = data->ref_date.now( simulation );

			std::string dd = data->ref_date.toDate(jd);

			ss.precision(10);
			ss << "Date set to ";
			ss << dd << " jd " << jd <<  std::endl;

			std::stringstream the_data;

			{

				std::vector < std::string > v1;
				utils::split( dd, " ", v1 );
				std::string date = v1[0];
				std::string time = v1[1];

				std::vector < std::string > v2;
				utils::split( date, "-", v2 );
				std::string day = v2[0];
				if ( day.size() == 1 )
					day = "0" + day;
				std::string month = v2[1];
				if ( month.size() == 1 )
					month = "0" + month;
				std::string year = v2[2];
				if ( year.size() != 4 ) {
					year = "0000" + month;
					year = year.substr( year.size() - 4 );
				}

				std::vector < std::string > v3;
				utils::split( time, ":", v3 );
				std::string hours = v3[0];
				if ( hours.size() == 1 )
					hours = "0" + hours;
				std::string minutes = v3[1];
				if ( minutes.size() == 1 )
					minutes = "0" + minutes;
				std::string seconds = v3[2];
				if ( seconds.size() == 1 )
					seconds = "0" + seconds;


				the_data <<
					"<input type=\"text\" id=\"date\" value=\"" <<
					day << "-" << month << "-" << year << "\" />" <<
					"<input type=\"text\" id=\"time\" value=\"" << hours << ":" << minutes << "\" />"
				;

			}

			std::stringstream xml;

			xml <<
				"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
				"<results>" <<
					"<terminal>" << ss.str() << "</terminal>" <<
					"<data>" << the_data.str() << "</data>" <<
				"</results>"
			;

			return ( xml.str() );


		}
		else if ( action == "scale" ) {

			if ( !cc.eof() ) {

				double value ;
				cc >> value;
				data->scale = value;

			}

			std::stringstream ss ;
			ss << "Scale value is set to ";
			ss << data->scale << std::endl;

			return ( ss.str() );

		}
		else if ( action == "refresh" ) {

			std::string refresh;
			cc >> refresh;

			if ( !cc.eof() ) {

				if ( utils::isNumber( refresh ) )
					data->refresh = atoi(refresh.c_str() );

			}

			std::stringstream ss ;
			ss << "Refresh set to ";
			ss << data->refresh << std::endl;

			return ( ss.str() );

		}
		else if ( action == "light" ) {

			std::string ambient;
			cc >> ambient;

			if ( !cc.eof() ) {

				if ( ambient == "true" ||  ambient == "on" )
					data->ambient_light = true;
				else if ( ambient == "false" ||  ambient == "off" )
					data->ambient_light = false;

			}


			std::stringstream ss ;
			data->ambient_light ? ss << "Ambient light set to on" : ss << "Ambient light set to off";


			// luce ambiente
			std::string ambient_light = "";
			if ( data->ambient_light ) {
				ambient_light = "	<light name=\"ambient\" type=\"ambient\" color=\"0x323232\" />";
			}
			else {
				ambient_light = "	<light name=\"ambient\" type=\"ambient\" color=\"0x323232\"><hidden /></light>";
			}

			std::stringstream xml;
			xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
						"<results>" <<
							"<renderer>" <<
								ambient_light <<
							"</renderer>"  <<
				"	<terminal>" <<
				ss.str() <<
				"	</terminal>"  <<
						"</results>" ;

			// manda i dati al client
			std::string type = "text";
			return ( xml.str() );

		}
		else if ( action == "execute_loop" ) {

			std::string start;

			if ( !cc.eof() ) {
				cc >> start;

				if ( start == "true" ||  start == "on" )
					data->executeLoop = true;
				else if ( start == "false" ||  start == "off" )
					data->executeLoop = false;

			}


			std::stringstream ss ;
			data->executeLoop ? ss << "Loop attivo" : ss << "Loop disattivo";


			std::stringstream xml;
			xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
						"<results>" <<
				"	<terminal>" <<
				ss.str() <<
				"	</terminal>"  <<
						"</results>" ;

			// manda i dati al client
			return ( xml.str() );

		}
		else if ( action == "eclittica" || action == "ecliptic" ) {

			std::string value;
			cc >> value;

			if ( !cc.eof() ) {

				SkyGrid & skyGrid = *data->skyGrid;

				if ( value == "false" ) {
					data->ecliptic = skyGrid.trackEcliptic(true);
				}
				else {
					data->ecliptic = skyGrid.trackEcliptic(false);
				}

				short show_ecliptic = value == "false" ? 0 : 1;
        data->showEcliptic = show_ecliptic == 1;
        
				std::stringstream xml;
				xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << 
						"<results>" << 
							"<data>" <<
								"<input value=\"" << show_ecliptic << "\" id=\"show_ecliptic\" type=\"checkbox\" />" << 
							"</data>" <<
						"	<renderer>" <<
						data->ecliptic <<
						"	</renderer>"  <<
						"</results>" ;

				// manda i dati al client
				return ( xml.str() );

			}

		}
		else if ( action == "render" ) {

//			return render( sockfd );
//			return do_sendLoop( sockfd );
			std::string xml = do_sendLoop( sockfd );
			return xml;

		}
		else if ( action == "equatoriale" || action == "equatorial" ) {

			std::string value;
			cc >> value;

			if ( !cc.eof() ) {

				std::string equatorial;
				SkyGrid & skyGrid = *data->skyGrid;

				if ( value == "false" )
					equatorial = skyGrid.trackEquatorial(true);
				else
					equatorial = skyGrid.trackEquatorial(false);

				std::stringstream xml;
				xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
						"<results>" <<
						"	<renderer>" <<
equatorial <<
						"	</renderer>"  <<
						"</results>" ;

				// manda i dati al client
				return ( xml.str() );

			}

		}
		else if ( action == "horizont" ) {


			std::string value;

			if ( !cc.eof() ) {
				cc >> value;
				if ( value == "false" )
					data->horizont = false;
				else
					data->horizont = true;


			}

			double latitude = data->lookfrom.latitude;
			double longitude = data->lookfrom.longitude;
			Horizont horizont ( latitude, longitude + data->rotation );

			std::stringstream track_horizont ;
			std::string track = horizont.track ( data->horizont );

			short show_horizont = data->horizont ? 1 : 0;
			std::stringstream xml;
			xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
						"<results>" <<
							"<data>" <<
								"<input value=\"" << show_horizont << "\" id=\"show_horizont\" type=\"checkbox\" />" <<
							"</data>" <<
						"	<renderer>" <<
track <<
						"	</renderer>"  <<
						"</results>" ;

			// manda i dati al client
			return ( xml.str() );

		}
		else if ( action == "axis" ) {

			std::string value;
			cc >> value;

			if ( !cc.eof() ) {

				std::stringstream xml;
				if ( value == "false" ) {

					xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
						"<results>" <<
						"	<renderer>" <<
						"<axis name=\"axis\"><hidden /></axis>" <<
						"	</renderer>"  <<
						"</results>" ;
				}
				else {
					xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
						"<results>" <<
						"	<renderer>" <<
						"<axis name=\"axis\"><show /></axis>" <<
						"	</renderer>"  <<
						"</results>" ;
				}


				// manda i dati al client
				return ( xml.str() );

			}

		}
		else if ( action == "orbitControl" ) {

			std::string c;
			cc >> c;

			std::string orbitControls ;
			if ( c == "1" ) {
				data->orbitControls = true;
				data->refresh = 3600;
				orbitControls = "<orbitControls />" ;
			}
			else {
				data->orbitControls = false;
				data->refresh = 0;
				orbitControls = "<orbitControls><remove /></orbitControls>";
			}


			std::stringstream xml;

			xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
				"<results>" <<

				"<renderer name=\"solar\">" <<

				orbitControls <<


				"</renderer>" <<


				"</results>" ;

			// manda i dati al client
			return ( xml.str() );

		}
		else if ( action == "simulation" ) {

			double simulation;
			cc >> simulation;

			JDay jd = data->ref_date.now( data->simulation );

			data->simulation = simulation;
			data->ref_date = jd;

			std::stringstream xml;
			xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
				"<results>" <<
					"<data>" <<
					"<span value=\"" << data->simulation << "\" id=\"span_simul\" />" <<
					"<ui value=\"" << data->simulation << "\" id=\"simul\" type=\"slider\" />" <<
					"</data>" <<
				"	<terminal>" <<
				"Simulation set to " << data->simulation <<
				"	</terminal>"  <<
				"</results>" ;

			return ( xml.str() );

		}
		else if ( action == "default" ) {

			data->set_default ();

			std::stringstream xml;
			xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
			"<results>" <<
				"<data>" <<
					"<span value=\"" << data->camera.fov << "\" id=\"span_fov\" />" <<
					"<ui value=\"" << data->camera.fov << "\" id=\"fov\" type=\"slider\" />" <<
					"<span value=\"" << data->camera.zoom << "\" id=\"span_zoom\" />" <<
					"<ui value=\"" << 10 * data->camera.zoom << "\" id=\"zoom\" type=\"slider\" />" <<
					"<span value=\"" << data->simulation << "\" id=\"span_simul\" />" <<
					"<ui value=\"" << data->simulation << "\" id=\"simul\" type=\"slider\" />" <<
				"</data>" <<
				"	<terminal>" <<
				"Default values set" <<
				"	</terminal>"  <<
			"</results>" ;



			return "";

		}
		else if ( action == "fov" ) {
			double fov;
			cc >> fov;
			if ( !cc.eof() ) {
				data->camera.fov = fov;
			}

			std::stringstream xml;
			xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
				"<results>" <<
					"<data>" <<
					"<span value=\"" << fov << "\" id=\"span_fov\" />" <<
					"<ui value=\"" << data->camera.fov << "\" id=\"fov\" type=\"slider\" />" <<
					"</data>" <<
				"	<terminal>" <<
				"Camera fov set to " << data->camera.fov <<
				"	</terminal>"  <<
				"</results>" ;

			return ( xml.str() );

		}
		else if ( action == "zoom" ) {
			std::string zoom;
			cc >> zoom;
			if ( !cc.eof() ) {
				if ( utils::isNumber( zoom ) ) {
					double val = atof( zoom.c_str() );
					if ( val > 0 )
						data->camera.zoom = val;
				}
				else {
					if ( utils::trim( zoom ) == "+" ) {
						data->camera.zoom *= 2 ;
					}
					else if ( utils::trim(zoom) == "-" ) {
						data->camera.zoom /= 2;
					}
				}
			}
			
			// ← FIX: Calcola scale inverso per sky sphere
			double skyScale = 1.0 / data->camera.zoom;
			
			std::stringstream xml;
			xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
				"<results>" <<
					"<renderer name=\"solar\">" <<
						// ← Aggiungi primitive sky con scale inverso
						"<primitive name=\"sky\" type=\"sphere\">" <<
							"<scale x=\"" << skyScale << "\" y=\"" << skyScale << "\" z=\"" << skyScale << "\" />" <<
						"</primitive>" <<
					"</renderer>" <<
					"<data>" <<
						"<span value=\"" << data->camera.zoom << "\" id=\"span_zoom\" />" <<
						"<ui value=\"" << 10 * data->camera.zoom << "\" id=\"zoom\" type=\"slider\" />" <<
					"</data>" <<
					"<terminal>" <<
						"Camera zoom set to " << data->camera.zoom << 
						" (sky scale: " << skyScale << ")" <<
					"</terminal>"  <<
				"</results>" ;
			return ( xml.str() );
		}

		else if ( action == "near" ) {
			double near;
			cc >> near;
			if ( !cc.eof() ) {
				data->camera.near = near;
			}

			std::stringstream ss ;
			ss.precision(12);
			ss << "Camera near set to ";
			ss << data->camera.near << std::endl;

			return ( ss.str() );

		}
		else if ( action == "far" ) {
			double far;
			cc >> far;
			if ( !cc.eof() ) {
				data->camera.far = far;
			}

			std::stringstream ss ;
			ss << "Camera far set to ";
			ss << data->camera.far << std::endl;

			return ( ss.str() );

		}
		else if ( action == "factor" ) {

			double fact;
			cc >> fact;
			if ( !cc.eof() ) {

				std::string name;
				cc >> name;
				if ( !cc.eof() ) {
					auto it = data->system->bodies.find( name );
					if ( it != data->system->bodies.end() ) {

						CelestialBody & body = it->second;
						body.set_size_fact( fact );

						std::string type = "text";
						std::stringstream xml;
						xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
							"<results>" <<
							"<terminal>" <<
								name << " factor " << fact <<
							"</terminal>" <<
							"<renderer>" <<
							"	<primitive name=\"" << it->first << "\" >"	<<
							"		<remove />"	<<
							"	</primitive>"	<<
							"</renderer>"  <<
							"</results>"  ;

						// manda i dati al client
						sendRFC6455 ( sockfd, type, xml.str(), false );

						// ricostruisce l'oggetto
						xml.str("");;
						xml <<
							"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
							"<results>" <<
							"<renderer>" <<
								body.primitive() <<
							"</renderer>"  <<
							"</results>"  ;

						return ( xml.str() );

					}

				}


			}

		}
		else if ( action == "tropical" || action == "tropic" ) {


			Date &d = data->ref_date;

			std::string a;
			cc >> a;

			if ( cc.eof() ) {
				return "";
			}
			else if ( a != "year" ) {
				return "";
			}

			short year ;
			cc >> year ;

			if ( cc.eof() ) {
				year = d.year();
			}

			Earth earth;
			double jd1 = earth.equinox(year, 3);
			double jd2 = earth.equinox(year-1,3);

			double tropic_year = jd1 - jd2;

			std::stringstream ss ;
			ss.precision(6);
			ss << "Tropical year " << year << " (vernal equinox) : " <<
				tropic_year << std::endl;

			return ( ss.str() );

		}
		else if ( action == "equinox" ) {


			Date &d = data->ref_date;
			short year ;
			cc >> year ;

			if ( cc.eof() ) {
				year = d.year();
			}

			std::stringstream ss ;
			Earth earth;


{

			try {

				double jm = earth.marchEquinox ( year );
				DT dm ( jm );
				UT utm ( dm );

				ss << "March equinox " <<" " <<
					"DT " << dm.toDate() <<" " <<
					"UT " << utm.toDate() << std::endl;

				double js = earth.septemberEquinox ( year );
				DT ds ( js );
				UT uts ( ds );

				ss << "September equinox " <<" " <<
					"DT " << ds.toDate() <<" " <<
					"UT " << uts.toDate() << std::endl;
			} catch ( const std::string & e ) {
				ss << e << std::endl;
			}
}
/*
{

			double jm = earth.equinox ( year, 3 );
			DT dm ( jm );
			UT utm ( dm );

			ss << "March equinox " << " " <<
				"DT " << dm.toDate() << " " <<
				"UT " << utm.toDate() << std::endl;

			double js = earth.equinox ( year, 9 );
			DT ds ( js );
			UT uts ( ds );

			ss << "September equinox " <<  " " <<
				"DT " << ds.toDate() << " " <<
				"UT " << uts.toDate() << std::endl;
}
*/

			return ( ss.str() );


/*
			Date &d = data->ref_date;
			short year ;
			cc >> year ;

			if ( cc.eof() ) {
				year = d.year();
			}

			std::stringstream ss ;
			Earth earth;

			double jm = earth.marchEquinox ( year );
			DT dm ( jm );

			ss << "March equinox " << std::endl;
			ss << "DT " << dm.toDate() << std::endl;
			UT utm ( dm );
			ss << "UT " << utm.toDate() << std::endl;

			double js = earth.septemberEquinox ( year );
			DT ds ( js );

			ss << "September equinox " << std::endl;
			ss << "DT " << ds.toDate() << std::endl;
			UT uts ( ds );
			ss << "UT " << uts.toDate() << std::endl;

			sendRFC6455 ( sockfd, "text", ss.str(), false );
*/

		}
		else if ( action == "solstice" ) {

			Date &d = data->ref_date;
			short year ;
			cc >> year ;

			if ( cc.eof() ) {
				year = d.year();
			}

			std::stringstream ss ;
			Earth earth;

			try {

				double j = earth.juneSolstice ( year );

				DT dt ( j );
				UT ut ( dt );

				ss << "June solstice " << " " <<
					"DT " << dt.toDate() << " " <<
					"UT " << ut.toDate() << std::endl;

			}
			catch ( const std::string & err) {
				ss << err << std::endl;
			}

			try {

				double j = earth.decemberSolstice ( year );

				DT dt ( j );
				UT ut ( dt );

				ss << "December solstice " << " " <<
					"DT " << dt.toDate() << " " <<
					"UT " << ut.toDate() << std::endl;

			}
			catch ( const std::string & err) {
				ss << err << std::endl;
			}
			catch ( ... ) {
			}

			return ( ss.str() );

		}
		else if ( action == "up" ) {
			clientData::Camera &camera = data->camera;
			std::string body;
			cc >> body;
			double x, y, z;
			cc >> x ;
			if ( !cc.eof() ) {
				cc >> y;
				if ( !cc.eof() ) {
					cc >> z;
					if ( !cc.eof() ) {
						if ( body == "camera" ) {
							camera.up_x = x ;
							camera.up_y = y ;
							camera.up_z = z ;
						}
						else {
							std::stringstream xml;
							xml.precision(15);
							xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
								"<results>" <<
								"	<renderer>" <<
								"<primitive name=\"" << body << "\">" <<
								"	<up x=\"" << x << "\" y=\"" << y << "\" z=\"" << z << "\" />"	<<
								"</primitive>"	<<
								"	</renderer>"  <<
								"</results>" ;

							return ( xml.str() );

						}
					}
				}
			}

/*
			std::stringstream ss ;
			ss << "up " << body << " " << camera.up_x <<
				" " << camera.up_y <<
				" " << camera.up_z;

			sendRFC6455 ( sockfd, "text", ss.str(), false );
*/

		}
		else if ( action == "lookAt" ) {

			std::string lookat;
			cc >> lookat;
			double x, y, z;
			cc >> x ;
			if ( !cc.eof() ) {
				cc >> y;
				if ( !cc.eof() ) {
					cc >> z;
				}
			}

			std::stringstream ss ;
			ss << "lookat " << lookat << " " << x <<
				" " << y <<
				" " << z;

//			sendRFC6455 ( sockfd, "text", ss.str(), false );

			std::stringstream xml;
			xml.precision(15);
			xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
				"<results>" <<
				"	<renderer>" <<
					"<primitive name=\"" << lookat << "\">" <<
					"	<lookat x=\"" << x << "\" y=\"" << y << "\" z=\"" << z << "\" />"	<<
					"</primitive>"	<<
				"	</renderer>"  <<
				"</results>" ;


			return ( ss.str() );

		}
		else if ( action == "lookat" ) {

			std::stringstream result;
			result.precision(12);


			std::string lookat;
			cc >> lookat;
			if ( lookat == "earth" ) {
				if ( !cc.eof() ) {

					std::string vista;
					cc >> vista;

					if ( vista == "moon" )
						data->lookat.where = "earth moon";
					else
						data->lookat.where = "earth";

				}
				else
					data->lookat.where = "earth";
			}
			else if ( lookat == "moon" ) {

				data->lookat.where = "moon";


/*
				Moon moon;
				OrbitEcli orbit = moon.orbitAlfaELP( JD );
				Grade alfa = orbit.lambda;
				Grade delta = orbit.beta; // declinazione
				double R = orbit.delta;
				result << "alfa =" << alfa <<
					" delta=" << delta <<
					" R=" << R << std::endl;
*/
			}
			else if ( lookat == "sun" ) {


				UT uut = d;
				DT ddt = uut;
				double JD = uut.now( simulation );

				Sun sun;

				OrbitEcli orbit_sun = sun.orbitLBR( JD );	// coordinate eclittiche

				OrbitEqu   o_sun ( JD );
				// converte le coordinate eclittiche in equatoriali
				o_sun = orbit_sun;

				Grade alfa_sun = o_sun.alfa;	// AR
				Grade delta_sun = o_sun.delta; // declinazione


				OrbitXYZ o_xyz = o_sun;	// converte in coordinate rettangolari
				double x_sun = o_xyz.x ;
				double y_sun = o_xyz.y ;
				double z_sun = o_xyz.z ;

				Point p1 (0, 0, 0);
				Point p2 ( x_sun, y_sun, z_sun );

				double r = p1.distance( p2 );

				StraightLine line ( p2, p1);

				Point p = line.pointAtDistance( r * 0.999 );

				clientData::Camera &camera = data->camera;

				camera.pos_x = p.x ;
				camera.pos_y = p.y ;
				camera.pos_z = p.z ;

				data->lookat.where = "sun";

			}
			else if ( lookat == "sky" )
				data->lookat.where = "sky";
			else {
				data->lookat.where = lookat;
			}

			std::stringstream ss ;
			ss << "Look at ";
			ss << data->lookat.where << std::endl;
			ss << result.str();

			return ( ss.str() );

		}
		else if ( action == "height" ) {

			double altezza;
			cc >> altezza;

			if ( !cc.eof() ) {

				data->lookat.altezza = altezza;

			}

			std::stringstream ss ;
			ss << "Altezza ";
			ss << data->lookat.altezza <<
				" gradi sopra l'orizzonte" << std::endl;

			return ( ss.str() );

		}
		else if ( action == "azimut" ) {

			double azimut;
			cc >> azimut;

			if ( !cc.eof() ) {

				data->lookat.azimut = azimut;
				data->lookat.view = clientData::lookAt::VIEW::AZIMUT;

			}

			std::stringstream ss ;
			ss << "Azimut set to ";
			ss << data->lookat.azimut <<  std::endl;

			return ( ss.str() );

		}
		else if ( action == "precession" ) {

			double x1, y1, z1;
			cc >> x1 >> y1 >> z1;
			UT uut = d;
			DT ddt = uut;
			double JD = uut.now( simulation );

			Earth & earth = data->system->earth ;
//			double rot = earth.rotation( JD ) ;
			Earth::Precession precession = earth.precession(0, 90, JD);
			Point coord = earth.bodyCoord ( precession.delta, precession.alfa );

			double x = coord.x / CelestialBody::AU;
			double y = coord.y / CelestialBody::AU;
			double z = coord.z / CelestialBody::AU;

			std::stringstream xml;
			xml.precision(15);
			xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
				"<results>" <<
"<terminal>delta=" << precession.delta << " alfa=" << precession.alfa <<
" x=" << x << " y=" << y << " z=" << z <<
"</terminal>" <<
				"	<renderer>" <<
					"<primitive name=\"" << "sky" << "\">" <<
					"	<lookat x=\"" << x1 << "\" y=\"" << y1 << "\" z=\"" << z1 << "\" />"	<<
					"</primitive>"	<<
				"	</renderer>"  <<
				"</results>" ;


			return ( xml.str() );

		}
		else if (action == "view") {
			std::string token;
			double azimut = 0.0;
			double altezza = 0.0;
			bool has_direction = false;
			bool has_angles = false;

			// Leggiamo il primo token dopo "view"
			if (cc >> token) {
				// Tentiamo prima di interpretarlo come direzione cardinale
				if (token == "N" || token == "NE" || token == "NW" ||
					token == "S" || token == "SE" || token == "SW" ||
					token == "E" || token == "W" || token == "O" || token == "Z") {

					has_direction = true;

					if (data->lookfrom.where == "earth") {
						data->lookat.where = "sky";

						if (token == "N") {
							data->lookat.view = clientData::lookAt::VIEW::NORTH;
						}
						else if (token == "NE") {
							data->lookat.view = clientData::lookAt::VIEW::NORTH_EAST;
						}
						else if (token == "NW") {
							data->lookat.view = clientData::lookAt::VIEW::NORTH_WEST;
						}
						else if (token == "SE") {
							data->lookat.view = clientData::lookAt::VIEW::SOUTH_EAST;
						}
						else if (token == "SW") {
							data->lookat.view = clientData::lookAt::VIEW::SOUTH_WEST;
						}
						else if (token == "S") {
							data->lookat.view = clientData::lookAt::VIEW::SOUTH;
						}
						else if (token == "E") {
							data->lookat.view = clientData::lookAt::VIEW::EAST;
						}
						else if (token == "W" || token == "O") {
							data->lookat.view = clientData::lookAt::VIEW::WEST;
						}
						else if (token == "Z") {
							data->lookat.view = clientData::lookAt::VIEW::ZENIT;
						}
						else {
							data->lookat.view = clientData::lookAt::VIEW::NORTH; // default
						}
					}
				}
				// Se non è una direzione, proviamo a leggerlo come numero (azimut)
				else {
					std::istringstream iss(token);
					if (iss >> azimut) {
						// Proviamo a leggere anche l'altezza (secondo float)
						if (cc >> altezza) {
							has_angles = true;

							if (data->lookfrom.where == "earth") {
								data->lookat.where = "sky";
								data->lookat.azimut = azimut;
								data->lookat.altezza = altezza;
								// Opzionale: resettiamo la view enum se usi anche gli angoli
								// data->lookat.view = clientData::lookAt::VIEW::CUSTOM; // se hai questo valore
								data->lookat.view = clientData::lookAt::VIEW::NONE;
//								set_horizont( data );
							}
						}
						else {
							// C'è solo un numero → probabilmente errore o uso parziale
							// Decidi tu cosa fare (per ora ignoriamo o usiamo come azimut con altezza 0)
							if (data->lookfrom.where == "earth") {
								data->lookat.where = "sky";
								data->lookat.azimut = azimut;
								data->lookat.altezza = 0.0;
							}
						}
					}
					// Se non è né direzione né numero, ignora o usa default
				}
			}

			// === Costruzione della risposta XML ===
			std::stringstream ss;
			ss << "view ";

			if (has_angles) {
				ss << "azimut=" << azimut << " altezza=" << altezza;
			}
			else if (has_direction) {
				ss << token;                    // rimandiamo la direzione letta
			}
			else {
				ss << "N";                      // default
			}

			ss << std::endl
			   << data->camera.lookat_x << " "
			   << data->camera.lookat_y << " "
			   << data->camera.lookat_z << std::endl;

			std::stringstream xml;
			xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"
				<< "<results>"
				<< "<terminal>" << ss.str() << "</terminal>"
				<< "</results>";

			return xml.str();
		}

		else if ( action == "hit" ) {

			Sky sky;

			double ascensione_retta ;
			double declinazione ;

			cc >> ascensione_retta ;

			std::stringstream ss ;

			if ( cc.eof() ) {

				ss << "Please set RA (hours) and Dec (degree)" << std::endl;

			}
			else {

				cc >> declinazione;

				if ( cc.eof() ) {
          ss << "Please set RA (hours) and Dec (degree)" << std::endl;
				}
				else {

					std::string hit = data->skyGrid->hitPoint ( ascensione_retta *  360 / 24, declinazione );

					std::stringstream xml;
					xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
						"<results>" <<
						"	<renderer>" <<
						hit <<
						"	</renderer>"  <<
						"<terminal>" <<
						hit <<
						"</terminal>" <<
						"</results>" ;

					// manda i dati al client
					return ( xml.str() );

				}

			}

			std::stringstream xml;
			xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
				"<results>" <<
						"<terminal>" <<
						ss.str() <<
            "</terminal>" <<
        "</results>" ;

			return ( xml.str() );


		}
		else {

			std::stringstream result;
			result.precision(12);

			clientData::Camera &camera = data->camera;
			auto it = data->system->bodies.find( action );
			if ( it != data->system->bodies.end() ) {

				CelestialBody & body = it->second;

				if ( !cc.eof() ) {

					std::string str;
					cc >> str ;

					std::stringstream xml;
					xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
						"<results>" <<
						"	<renderer>" <<
						"		<primitive name=\"" << it->first << "\" >"	;

					if ( str == "hidden" || str == "hide"  ) {

							xml << "	<hidden />" ;

					}
					else if ( str == "show" ) {

						xml << "<show />" ;
					}
					xml << "	</primitive>"	<<
						"	</renderer>"  <<
						"</results>" ;

					// manda i dati al client
					return ( xml.str() );

				}
				else {

					// specifica le caratteristiche del corpo celeste

					OrbitEqu o = body.position;
					double ar = o.alfa  ;
					double dec = o.delta;

					if ( ar < 0 )
						ar = 360 + ar  ;

					SkyGrid & skyGrid = *data->skyGrid;
					std::string constellation = skyGrid.whichConstellation ( ar, dec ) ;
					std::string zodiaco = skyGrid.whichZodiac ( ar );

					result << it->first << " in " << constellation <<
						" [" << zodiaco << "]" << std::endl <<
						"\tDec=" << o.delta.toGradeGPS() << " AR=" << o.alfa.toGradeHMS() << " R=" << o.R << std::endl <<
						"\tDec=" << dec << " AR=" << ar << " R=" << o.R << std::endl ;

				}

			}
			else {

				UT ut = d.now( simulation );
				Date LT ( d.now( simulation ) + data->lookfrom.dstOffset / 24 );
				DT dt = ut ;

				std::stringstream cities;
				std::stringstream ss;
				if ( data->lookfrom.where == "earth" ) {

					double north = data->lookfrom.latitude + 0.05;
					double south = data->lookfrom.latitude - 0.05;
					double east = data->lookfrom.longitude + 0.05;
					double west = data->lookfrom.longitude - 0.05;

					std::stringstream query ;
					query << "north=" << north
						<< "&south=" << south
						<< "&east=" << east
						<< "&west=" << west
						<< "&username=baldoweb" ;

					TCPClient geoLoc ( "api.geonames.org" ) ;
					std::string geo_result = geoLoc.get( "/cities", query.str() );


					XMLDocument doc;
					doc.parse( geo_result );
					
					XMLElement * root = doc.rootElement() ;

					auto geoname =  root->getElementsByTagName ( "geoname" ) ;
					for ( size_t i = 0; i < geoname.size(); ++i ) {

						XMLElement *geo = geoname.at(i);
						XMLElement * e_name =  geo->getFirstElementByTagName ( "name" ) ;
						XMLElement * e_countryName = geo->getFirstElementByTagName ( "countryName" ) ;
						XMLElement * e_lat = geo->getFirstElementByTagName ( "lat" ) ;
						XMLElement * e_lng = geo->getFirstElementByTagName ( "lng" ) ;

						std::string countryName, name, lat, lng;
						if ( e_countryName )
							countryName = e_countryName->innerHTML();
						if ( e_name  )
							name = e_name->innerHTML();
						if ( e_lat  )
							lat = e_lat->innerHTML();
						if ( e_lng  )
							lng = e_lng->innerHTML();

						cities  << name.replace(name.end()-1, name.end(), "")
								<< ", " << countryName.replace(countryName.end()-1, countryName.end(),"");

						break;

					}


					{

						short y = d.year();
						short m = d.month();
						short g = d.day();
						std::stringstream query ;
						query << "lat=" << data->lookfrom.latitude
								<< "&lng=" << data->lookfrom.longitude
								<< "&username=baldoweb"
								<< "&date=" << y << "-" << m << "-" << g ;

						TCPClient geoLoc ( "api.geonames.org" ) ;
						std::string geo_result = geoLoc.get( "/timezone", query.str() );

						ss << "thanks to geonames.org" << std::endl;

						XMLDocument doc;
						doc.parse( geo_result );
						
						XMLElement * root = doc.rootElement() ;

						auto geoname =  root->getElementsByTagName ( "timezone" ) ;
						
						for ( size_t i = 0; i < geoname.size(); ++i ) {

							XMLElement *geo = geoname.at(i);
							XMLElement * e_timezoneId =  geo->getFirstElementByTagName ( "timezoneId" ) ;
							XMLElement * e_countryName = geo->getFirstElementByTagName ( "countryName" ) ;
							XMLElement * e_lat = geo->getFirstElementByTagName ( "lat" ) ;
							XMLElement * e_lng = geo->getFirstElementByTagName ( "lng" ) ;

							XMLElement * e_gmtOffset = geo->getFirstElementByTagName ( "gmtOffset" ) ;
							XMLElement * e_dstOffset = geo->getFirstElementByTagName ( "dstOffset" ) ;
							XMLElement * e_rawOffset = geo->getFirstElementByTagName ( "rawOffset" ) ;
							XMLElement * e_time = geo->getFirstElementByTagName ( "time" ) ;

							XMLElement * e_sunrise = geo->getFirstElementByTagName ( "sunrise" ) ;
							XMLElement * e_sunset = geo->getFirstElementByTagName ( "sunset" ) ;

							std::string timezoneId, countryName, lat,
								lng, gmtOffset, time, sunrise, sunset ;

							if ( e_countryName )
								ss << "\tCountry " << e_countryName->innerHTML();
							if ( e_timezoneId )
								ss << "\tZone " << e_timezoneId->innerHTML();
							if ( e_lat )
								ss << "\tLat " << e_lat->innerHTML();
							if ( e_lng )
								ss << "\tLng " << e_lng->innerHTML();
							if ( e_gmtOffset ) {
								ss << "\tgmtOffset " << e_gmtOffset->innerHTML();
							}
							if ( e_dstOffset ) {
								ss << "\tdstOffset " << e_dstOffset->innerHTML();
							}
							if ( e_rawOffset ) {
								ss << "\trawOffset " << e_rawOffset->innerHTML();
								data->lookfrom.dstOffset = atof( e_rawOffset->innerHTML().c_str() );
							}
							if ( e_time )
								ss << "\tTime " << e_time->innerHTML();
							if ( e_sunrise )
								ss << "\tSunrise " << e_sunrise->innerHTML();
							if ( e_sunset )
								ss << "\tSunset " << e_sunset->innerHTML();

							ss << std::endl;


						}

					}

				}


				result <<
					"UT " << ut.toDate() <<  "\r\n" <<
					"LT " << LT.toDate() << " (add DST if needed) " <<
					"DT " << dt.toDate()  << " " << "\r\n" <<
					"JD " << dt.julianDay() << "\r\n" <<
					"look from " << data->lookfrom.where << "\r\n" <<
					"\t" << cities.str() << "\r\n" <<
					"\t" << ss.str() << "\r\n" <<
					"\tx=" << camera.pos_x << " y=" << camera.pos_y << " z=" << camera.pos_z << "\r\n" <<
					"look to " << data->lookat.where << "\r\n" <<
					"\tx=" << camera.lookat_x << " y=" << camera.lookat_y << " z=" << camera.lookat_z << "\r\n" ;

			}

			std::stringstream xml;

			xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
					"<results>" <<
						"<terminal>" <<
						result.str() <<
						"</terminal>" <<
					"</results>" ;



			return ( xml.str() );

		}

	}

	return "";

}


std::string WSsrv::strActualDate( int id )
{

	auto it = clients.find ( id );
	Client * client = it->second;

	clientData * data = reinterpret_cast < clientData * > ( client->extra );

	// accelera la simulazione di data->simulation secondi
	short simulation = data->simulation;
	// la data di riferimento
	Date &d = data->ref_date;
	UT uut = d;
	double JD = uut.now( simulation ) ;
    
  return d.toDate( JD );

}

std::string  WSsrv::do_sendLoop ( int id  )
{

	auto it = clients.find ( id );
	Client * client = it->second;

	clientData * data = reinterpret_cast < clientData * > ( client->extra );

	if ( !data->executeLoop ) {
//		std::cout << "executeLoop non attivato!" << std::endl;
		return "";
	}

	if ( !data ) {
		std::cout << "Non ci sono data per questo client" << std::endl;
		return "";
	}

	// scandisce il refresh dei dati grafici a monitor
	Timer & timer = data->timer;
	const auto elapsed = timer.time_elapsed();
	std::chrono::milliseconds msecs = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);

	// scandisce la scrittura dei dati a terminale
	Timer & timer_result = data->timer_result;
	const auto elapsed_res = timer_result.time_elapsed();
	std::chrono::milliseconds msecs_res = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_res);

	if ( !timer.isRunning() || msecs > std::chrono::milliseconds( 1000 * data->refresh ) ) {

		// accelera la simulazione di data->simulation secondi
		short simulation = data->simulation;
		// la data di riferimento
		Date &d = data->ref_date;
		// giorno giuliano attuale, la data + tempo passato dallo start
		// aggiustato con il tempo di simulazione
		UT uut = d;
		DT ddt = uut;
		double JD = uut.now( simulation ) ;

		SkyGrid & skyGrid = *data->skyGrid;
		skyGrid.erasePlanetsFromIndex ( ) ;

		// resetta il timer
		timer.start();

		Sun & sun = data->system->sun ;

		OrbitEcli orbit_sun = sun.orbitLBR( JD );

		OrbitEqu   o_sun ( JD );
		// converte le coordinate eclittiche in equatoriali
		o_sun = orbit_sun;

		Grade alfa_sun = o_sun.alfa;	// AR
		Grade delta_sun = o_sun.delta; // declinazione

		// converte in coordinate rettangolari
		OrbitXYZ o_sun_xyz = o_sun;
		double x_sun = o_sun_xyz.x ;
		double y_sun = o_sun_xyz.y ;
		double z_sun = o_sun_xyz.z ;

		sun.position = o_sun_xyz ;

		Earth & earth = data->system->earth ;
		double r_earth = earth.radius / CelestialBody::AU ;
		double rot = earth.rotation ( ddt, data->simulation ) ;
		data->rotation = rot;
		OrbitXYZ orbit_earth_xyz ( 0, 0, 0 ) ;
		orbit_earth_xyz.rot_y = rot ;
		Grade epsilon = earth.obliquity ( JD );
		earth.obliq = epsilon;

		earth.position = orbit_earth_xyz ;

		Moon & moon = data->system->moon ;
		double r_moon = moon.radius / CelestialBody::AU ;

		// i dati della teoria ELP
		OrbitEcli orbit_moon = moon.orbitAlfaELP( JD );
		Grade lambda_moon = orbit_moon.L;
		Grade beta_moon = orbit_moon.B;

		OrbitEqu   o_moon ( JD );
		// converte le coordinate eclittiche in equatoriali
		o_moon = orbit_moon;

		Grade alfa_moon = o_moon.alfa;
		Grade delta_moon = o_moon.delta; // declinazione


		// converte in coordinate rettangolari
		OrbitXYZ o_moon_xyz = o_moon;
		double x_moon = o_moon_xyz.x ;
		double y_moon = o_moon_xyz.y ;
		double z_moon = o_moon_xyz.z ;
		o_moon_xyz.rot_x += 1.543 ;

		// orientamento della luna rispetto alla terra
		// devo ruotare esattamente di alfa_moon
		//		Grade rot_luna = orbit_moon.lambda;
		// TODO non funziona ancora
		Grade rot_luna =  180 + alfa_moon;
		o_moon_xyz.rot_y = rot_luna ;

		moon.position = o_moon_xyz ;

		/*
		l'orbita dei pianeti è calcolata con VSOP C (rettangolare ecclittica eliocentrica)
		* convertita ad equatoriale geocentrica
		utilizzando l'orbita rettangolare del sole e eccentricità terra
		*/
		OrbitXYZ orbit_sun_xyz = sun.orbit ( MainBody::earth_data, JD ) ;

		Radiant eps = epsilon;

		Mercury & mercury  = data->system->mercury  ;
		OrbitXYZ orbit_mercury_xyz = mercury.orbit_eq ( MainBody::mercury_data, orbit_sun_xyz, epsilon, JD ) ;
		mercury.position = orbit_mercury_xyz ;

		{
			OrbitEqu o_eq = mercury.position;
			skyGrid.insertPlanetIntoIndex ( 1, o_eq.alfa, o_eq.delta );
		}


		Venus & venus  = data->system->venus  ;
		OrbitXYZ orbit_venus_xyz = venus.orbit_eq ( MainBody::venus_data, orbit_sun_xyz, epsilon, JD ) ;
		venus.position = orbit_venus_xyz ;

		{
			OrbitEqu o_eq = venus.position;
			skyGrid.insertPlanetIntoIndex ( 2, o_eq.alfa, o_eq.delta );
		}



		Mars & mars  = data->system->mars  ;
		OrbitXYZ orbit_mars_xyz = mars.orbit_eq ( MainBody::mars_data, orbit_sun_xyz, epsilon, JD ) ;

		double x_mars = orbit_mars_xyz.x;
		double y_mars = orbit_mars_xyz.y;
		double z_mars = orbit_mars_xyz.z;
		mars.position = orbit_mars_xyz ;

		{
			OrbitEqu o_eq = mars.position;
			skyGrid.insertPlanetIntoIndex ( 4, o_eq.alfa, o_eq.delta );
		}


		Jupiter & jupiter  = data->system->jupiter  ;
		OrbitXYZ orbit_jupiter_xyz = jupiter.orbit_eq ( MainBody::jupiter_data, orbit_sun_xyz, epsilon, JD ) ;

		double x_jupiter = orbit_jupiter_xyz.x ;
		double y_jupiter = orbit_jupiter_xyz.y ;
		double z_jupiter = orbit_jupiter_xyz.z ;
		jupiter.position = orbit_jupiter_xyz ;
		orbit_jupiter_xyz.rot_x += jupiter.obliquity ( JD ) ;

		{
			OrbitEqu o_eq = jupiter.position;
			skyGrid.insertPlanetIntoIndex ( 5, o_eq.alfa, o_eq.delta );
		}

		Io & io = data->system->io ;
		OrbitXYZ orbit_io_xyz = io.orbit ( JD ) ;
		orbit_io_xyz.x += x_jupiter ;
		orbit_io_xyz.y += y_jupiter ;
		orbit_io_xyz.z += z_jupiter ;

		// orientamento di io rispetto jupiter
		// devo ruotare di atan(y/x)

		io.position = orbit_io_xyz ;

		Europa & europa = data->system->europa ;
		OrbitXYZ orbit_europa_xyz = europa.orbit ( JD ) ;
		orbit_europa_xyz.x += x_jupiter ;
		orbit_europa_xyz.y += y_jupiter ;
		orbit_europa_xyz.z += z_jupiter ;
		europa.position = orbit_europa_xyz ;

		Ganimede & ganimede = data->system->ganimede ;
		OrbitXYZ orbit_ganimede_xyz = ganimede.orbit ( JD ) ;
		orbit_ganimede_xyz.x += x_jupiter ;
		orbit_ganimede_xyz.y += y_jupiter ;
		orbit_ganimede_xyz.z += z_jupiter ;
		ganimede.position = orbit_ganimede_xyz ;

		Callisto & callisto = data->system->callisto ;
		OrbitXYZ orbit_callisto_xyz = callisto.orbit ( JD ) ;
		orbit_callisto_xyz.x += x_jupiter ;
		orbit_callisto_xyz.y += y_jupiter ;
		orbit_callisto_xyz.z += z_jupiter ;
		callisto.position = orbit_callisto_xyz ;



		Saturn & saturn = data->system->saturn ;
		OrbitXYZ orbit_saturn_xyz = saturn.orbit_eq ( MainBody::saturn_data, orbit_sun_xyz, epsilon, JD ) ;
		saturn.position = orbit_saturn_xyz ;

		{
			OrbitEqu o_eq = saturn.position;
			skyGrid.insertPlanetIntoIndex ( 6, o_eq.alfa, o_eq.delta );
		}


		Uranus & uranus = data->system->uranus ;
		OrbitXYZ orbit_uranus_xyz = uranus.orbit_eq ( MainBody::uranus_data, orbit_sun_xyz, epsilon, JD ) ;
		uranus.position = orbit_uranus_xyz ;

		Neptune & neptune = data->system->neptune ;
		OrbitXYZ orbit_neptune_xyz = neptune.orbit_eq ( MainBody::neptune_data, orbit_sun_xyz, epsilon, JD ) ;

		neptune.position = orbit_neptune_xyz ;

		// imposta camera.lookat
		set_horizont( data );


		// il punto di osservazione
		clientData::Camera &camera = data->camera;
		double lookat_x = camera.lookat_x;
		double lookat_y = camera.lookat_y;
		double lookat_z = camera.lookat_z;

		double pos_x = camera.pos_x;
		double pos_y = camera.pos_y;
		double pos_z = camera.pos_z;

		double up_x = camera.up_x;
		double up_y = camera.up_y;
		double up_z = camera.up_z;

		double latitude = data->lookfrom.latitude;
		double longitude = data->lookfrom.longitude;
		Horizont horizont ( latitude, longitude + rot );

		std::stringstream track_horizont ;
		std::string track = horizont.track ( data->horizont );

		track_horizont << track ;

		if ( data->lookfrom.isFromPlace ) {


			std::stringstream xml;

			if ( data->lookfrom.where == "earth" ) {


				Point pdv = horizont.pointOfView();

				pos_x = pdv.x;
				pos_y = pdv.y;
				pos_z = pdv.z;

				camera.up_x = pos_x;
				camera.up_y = pos_y;
				camera.up_z = pos_z;

			}
			else if ( data->lookfrom.where == "moon" ) {

				Point coord = moon.bodyCoord ( latitude, longitude + rot_luna );

				pos_x = x_moon + coord.x / CelestialBody::AU;
				pos_y = y_moon + coord.y / CelestialBody::AU;
				pos_z = z_moon + coord.z / CelestialBody::AU;

			}
			else if ( data->lookfrom.where == "mars" ) {

				Point coord = mars.bodyCoord ( latitude, longitude );

				pos_x = x_mars + coord.x / CelestialBody::AU;
				pos_y = y_mars + coord.y / CelestialBody::AU;
				pos_z = z_mars + coord.z / CelestialBody::AU;

			}
			else if ( data->lookfrom.where == "sun" ) {

				Point coord = sun.bodyCoord ( latitude, longitude );

				pos_x = x_sun + coord.x / CelestialBody::AU;
				pos_y = y_sun + coord.y / CelestialBody::AU;
				pos_z = z_sun + coord.z / CelestialBody::AU;

			}
			else {

				auto it = data->system->bodies.find( data->lookfrom.where );
				if ( it != data->system->bodies.end() ) {

					CelestialBody & body = it->second;
					Point coord = body.bodyCoord ( latitude, longitude );

					pos_x = body.position.x + coord.x / CelestialBody::AU;
					pos_y = body.position.y + coord.y / CelestialBody::AU;
					pos_z = body.position.z + coord.z / CelestialBody::AU;

				}


			}


		}

//		double scale = data->scale;

		if ( data->lookat.where == "earth" ) {

			if ( !data->lookfrom.isFromPlace ) {

				// se non osservi da un punto particolare allora gurda dal sole
				Point p1 (x_sun, y_sun, z_sun);
				Point p2 ( 0, 0, 0 );

				double r = p1.distance( p2 );

				StraightLine line ( p1, p2);

				Point p = line.pointAtDistance( r * 0.995 );

				pos_x = p.x ;
				pos_y = p.y ;
				pos_z = p.z ;

			}

			lookat_x = 0;
			lookat_y = 0;
			lookat_z = 0;

		}
		else if ( data->lookat.where == "earth moon" ) {


				Point p1 ( x_moon, y_moon, z_moon );
				Point p2 ( 0, 0, 0 );

				double r = p2.distance( p1 );

				StraightLine line ( p2, p1);

				Point p = line.pointAtDistance( r - r_moon  );

			pos_x = p.x  ;
			pos_y = p.y ;
			pos_z = p.z ;

			lookat_x = 0;
			lookat_y = 0;
			lookat_z = 0;

		}
		else if ( data->lookat.where == "sky" ) {

			if ( !data->lookfrom.isFromPlace ) {

				// se non osservi da un punto particolare allora guarda dalla terra
				/*
mi posizione sulla crosta terrestre nella linea che
unisce il centro della terra con il corpo celesto
moltiplico per 1000 in quanto i valori sono troppo piccoli
e potrebbe convergere a 0
				 */
				Point p1 ( 0, 0, 0 );
				Point p2 ( 1000 * x_moon, 1000 * y_moon, 1000 * z_moon );

				double r = p2.distance( p1 );

				StraightLine line ( p2, p1);

				Point p = line.pointAtDistance( r - 1000 * r_earth  );

				pos_x = p.x / 1000 ;
				pos_y = p.y / 1000 ;
				pos_z = p.z / 1000 ;

			}

		}
		else if ( data->lookat.where == "sun" ) {

			lookat_x = x_sun ;
			lookat_y = y_sun ;
			lookat_z = z_sun ;

		}
		else {

			auto it = data->system->bodies.find( data->lookat.where );
			if ( it != data->system->bodies.end() ) {

				CelestialBody & body = it->second;


				if ( !data->lookfrom.isFromPlace ) {

// se non osservi da un punto particolare allora guarda dalla terra
				/*
mi posizione sulla crosta terrestre nella linea che
unisce il centro della terra con il corpo celesto
moltiplico per 1000 in quanto i valori sono troppo piccoli
e potrebbe convergere a 0
				 */
					double fact = 100 ;
					double r_body = body.radius / CelestialBody::AU;;
					Point p1 ( 0, 0, 0 );
					Point p2 ( fact * body.position.x, fact * body.position.y, fact * body.position.z );

					double r = p1.distance( p2 );

					StraightLine line ( p1, p2);

					Point p = line.pointAtDistance( r - fact * 100 * r_body  );

					pos_x = p.x / fact ;
					pos_y = p.y / fact ;
					pos_z = p.z / fact ;


				}

				lookat_x = body.position.x ;
				lookat_y = body.position.y ;
				lookat_z = body.position.z ;

			}

	}



		std::string hidden_sky;
		if ( data->camera.zoom < 0.1 || data->camera.zoom > 2 )
			hidden_sky = "<hidden />";	// inibisci il firmamento
		else
			hidden_sky = "";

		std::stringstream the_data;
		the_data.precision(12);
		the_data << "<div value=\"" << d.toDate( JD ) << "\" id=\"the_date\" />" <<
			 "<div value=\"" << JD << "\" id=\"the_jd\" />"
			 "<input type=\"checkbox\" id=\"a_light\" value=\"" << data->ambient_light << "\" />" <<
			 "<select id=\"sel_simulazione\" value=\"" << data->simulation << "\" />"
		 ;

		if ( data->lookfrom.where == "earth" ) {

			the_data <<
				"<input id=\"azimut\" value=\"" << data->lookat.azimut << "\" />" <<
				"<input id=\"height\" value=\"" << data->lookat.altezza << "\" />" <<
				"<div id=\"longitudine\" value=\"" << data->lookfrom.longitude << "\" />" <<
				"<div id=\"latitudine\" value=\"" << data->lookfrom.latitude << "\" />"
			;

		}

		std::string terminal;

		// alcuni dati li scrive solo dopo 30 sec, per esempio a terminale
		if ( !timer_result.isRunning() || msecs_res > std::chrono::milliseconds( 30000 ) ) {

			std::stringstream result;
			result.precision(12);

			timer_result.start();

			if ( result.str() != "" )
				terminal = "<terminal>" + result.str() + "</terminal>" ;
			else
				terminal = "";

//			the_data <<	"<input type=\"text\" id=\"altezza\" value=\"" << data->lookat.altezza << "\" />" ;
			 ;


		}





		std::stringstream e_fov, e_zoom, e_near, e_far ;
		if ( data->camera.fov != data->camera.old_fov )
			e_fov << "		<fov value=\"" << data->camera.fov << "\" />" ;
		if ( data->camera.zoom != data->camera.old_zoom )
			e_zoom << "		<zoom value=\"" << data->camera.zoom << "\" />" ;
		if ( data->camera.near != data->camera.old_near )
			e_near << "		<near value=\"" << data->camera.near << "\" />" ;
		if ( data->camera.far != data->camera.old_far )
			e_far << "		<far value=\"" << data->camera.far << "\" />" ;

		data->camera.old_fov = data->camera.fov;
		data->camera.old_zoom = data->camera.zoom;
		data->camera.old_far = data->camera.far;
		data->camera.old_near = data->camera.near;


		camera.lookat_x = lookat_x;
		camera.lookat_y = lookat_y;
		camera.lookat_z = lookat_z;

		camera.pos_x = pos_x;
		camera.pos_y = pos_y;
		camera.pos_z = pos_z;

		double FACT = data->FACT;

		std::string sun_primitive;
		{

			// distanza dalla terra
//			const double sun_distance = CelestialBody::AU * orbit_sun.R;
			// tempo che impiega la luce a percorcorre questa distanza in sec
//			const double t_sec = sun_distance / CelestialBody::LIGHT_SPEED;
			const double t_sec = 0;

			double JD_SUN = JD - t_sec / (24 * 60 * 60);
/*
	std::string t = timestamp(  );
    std::cout <<  std::fixed << std::setprecision(2) << t << " " << t_sec << std::endl ;
*/
			OrbitEcli orbit_sun = sun.orbitLBR( JD_SUN );

			OrbitEqu   o_sun ( JD_SUN );
			// converte le coordinate eclittiche in equatoriali
			o_sun = orbit_sun;

			// converte in coordinate rettangolari
			OrbitXYZ o_sun_xyz = o_sun;

			sun_primitive = sun.primitive_pos(o_sun_xyz);
			sun.position = o_sun_xyz ;

		}

		SolarSystem * system = data->system;
//		std::string primitives = system->primitive_pos();
		std::string primitives = system->primitive();
    
		std::string ecliptic = data->skyGrid->ecliptic( 2024 );


		OrbitXYZ sky_pos ( 0, 0, 0 );
		std::stringstream xml;

//	std::cout << JD <<  "   "  << JD_SUN << std::endl;

		xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<

			"<results>" <<

				terminal <<

				"<data>" << the_data.str() << "</data>" <<

				"<renderer name=\"solar\">" <<

				"	<camera>" <<
					"	<position x=\"" << FACT * pos_x << "\" y=\"" << FACT * pos_y << "\" z=\"" << FACT * pos_z << "\" />"	<<
				"		<lookat x=\"" << FACT * lookat_x << "\" y=\"" << FACT * lookat_y << "\" z=\"" << FACT * lookat_z << "\" />" <<
				"		<up x=\"" << up_x << "\" y=\"" << up_y << "\" z=\"" << up_z << "\" />" <<

				e_zoom.str() <<
				e_fov.str() <<
				e_near.str() <<
				e_far.str() <<

				"		<shadow />" <<
				"	</camera>" <<

					"<light  type=\"point\" color=\"0xffffff\" intensity=\"1.0\" name=\"sunlight\">" <<
					"	<position x=\"" << FACT * x_sun << "\" y=\"" << FACT * y_sun << "\" z=\"" << FACT * z_sun << "\" />"	<<
					"</light>" <<

				track_horizont.str() <<
        

				primitives <<
//				sun_primitive <<

				"<primitive type=\"point\" name=\"planets\">" <<
					"<recreate />" <<
					"<vertice>" << orbit_jupiter_xyz.x << " " << orbit_jupiter_xyz.y << " " << orbit_jupiter_xyz.z << "</vertice>" <<
					"<vertice>" << orbit_mercury_xyz.x << " " << orbit_mercury_xyz.y << " " << orbit_mercury_xyz.z << "</vertice>" <<
					"<vertice>" << orbit_mars_xyz.x << " " << orbit_mars_xyz.y << " " << orbit_mars_xyz.z << "</vertice>" <<
					"<vertice>" << orbit_venus_xyz.x << " " << orbit_venus_xyz.y << " " << orbit_venus_xyz.z << "</vertice>" <<
					"<vertice>" << orbit_saturn_xyz.x << " " << orbit_saturn_xyz.y << " " << orbit_saturn_xyz.z << "</vertice>" <<
					"<material size=\"3\" color=\"0xffffff\" type=\"point\" />" <<
				"</primitive>" <<
        
        ecliptic <<


				"</renderer>" <<


			"</results>" ;

    // manda i dati al client
//		return utils::compressToWebSocket(xml.str());

		return xml.str();


	}

	// non ritrasmetto più l'informazione
	data->tracciato = "" ;
	data->asterismi = "" ;

	std::cout << "Fine loop senza dati" << std::endl;
	return "";

}	// std::string  WSsrv::do_sendLoop ( int id  )


void WSsrv::do_disconect ( Client * client )
{

	clientData * data = reinterpret_cast < clientData * > ( client->extra );
	delete data ;

	client->extra = nullptr;


#ifdef DEBUG
{

		std::stringstream ss ;
		ss << "file " << __FILE__ << " funzione " << __func__ << std::endl;
		print_log( ss.str()) ;

}
#endif

}

void WSsrv::set_horizont ( clientData * data )
{

	double latitude = data->lookfrom.latitude;
	double longitude = data->lookfrom.longitude;

	double rot = data->rotation ;
	Horizont horizont ( latitude, longitude + rot );


	clientData::Camera &camera = data->camera;

	switch ( data->lookat.view ) {

		case clientData::lookAt::VIEW::NORTH :

			data->lookat.altezza = 0;
			data->lookat.azimut = 0 ;

			break;

		case clientData::lookAt::VIEW::SOUTH :

			data->lookat.altezza = 0;
			data->lookat.azimut = 180 ;

			break;


		case clientData::lookAt::VIEW::EAST :

			data->lookat.altezza = 0;
			data->lookat.azimut = 90 ;

			break;



		case clientData::lookAt::VIEW::WEST :

			data->lookat.altezza = 0;
			data->lookat.azimut = 270 ;

			break;



		case clientData::lookAt::VIEW::NORTH_EAST :

			data->lookat.altezza = 0;
			data->lookat.azimut = 45 ;

			break;


			break;


		case clientData::lookAt::VIEW::NORTH_WEST :

			data->lookat.altezza = 0;
			data->lookat.azimut = 270 + 45 ;

			break;


		case clientData::lookAt::VIEW::SOUTH_EAST :

			data->lookat.altezza = 0;
			data->lookat.azimut = 90 + 45;

			break;


		case clientData::lookAt::VIEW::SOUTH_WEST :

			data->lookat.altezza = 0;
			data->lookat.azimut = 180 + 45 ;

			break;


		case clientData::lookAt::VIEW::ZENIT : {

			Point pdv = horizont.pointOfView ( ) ;
			Point p ( 10 * pdv.x, 10 + pdv.y, 10 * pdv.z );
			camera.lookat_x = p.x ;
			camera.lookat_y = p.y ;
			camera.lookat_z = p.z ;

			}

			return ;

		case clientData::lookAt::VIEW::AZIMUT :
		case clientData::lookAt::VIEW::NONE :

			break;

		default :

			return ;


	}

	if ( data->lookat.altezza > 89 ) data->lookat.altezza = 89;
	if ( data->lookat.altezza < -89 ) data->lookat.altezza = -89;

	if ( data->lookat.azimut > 360 ) data->lookat.azimut -= 360;
	if ( data->lookat.azimut < 0 ) data->lookat.azimut += 360;

	Point pn = horizont.lookatAzimut ( data->lookat.azimut, data->lookat.altezza ) ;

	camera.lookat_x = pn.x;
	camera.lookat_y = pn.y;
	camera.lookat_z = pn.z ;


}

std::string WSsrv::render ( int id ) {

//	int sockfd = id ;

	auto it = clients.find ( id );
	Client * client = it->second;

	clientData * data = reinterpret_cast < clientData * > ( client->extra );
	if ( !data ) return "";


	// accelera la simulazione di data->simulation secondi
	short simulation = data->simulation;

	// la data di riferimento
	Date &d = data->ref_date;
	// giorno giuliano attuale, la data + tempo passato dallo start
	// aggiustato con il tempo di simulazione
	UT uut = d;
	DT ddt = uut;
	double JD = uut.now( simulation );


	Sun & sun = data->system->sun ;

	OrbitEcli orbit_sun = sun.orbitLBR( JD );
	OrbitEqu   o_sun ( JD );
	// converte le coordinate eclittiche in equatoriali
	o_sun = orbit_sun;

	Grade alfa_sun = o_sun.alfa;	// AR
	Grade delta_sun = o_sun.delta; // declinazione

	// converte in coordinate rettangolari
	OrbitXYZ o_sun_xyz = o_sun;
	double x_sun = o_sun_xyz.x ;
	double y_sun = o_sun_xyz.y ;
	double z_sun = o_sun_xyz.z ;

	sun.position = o_sun_xyz ;



	Earth & earth = data->system->earth ;
	double rot = earth.rotation ( ddt, data->simulation ) ;

	data->rotation = rot;

	OrbitXYZ orbit_earth_xyz ( 0, 0, 0 ) ;
	orbit_earth_xyz.rot_y = rot ;
	Grade epsilon = earth.obliquity ( JD );
	earth.obliq = epsilon;

	earth.position = orbit_earth_xyz ;

	Moon & moon = data->system->moon ;

	// i dati della teoria ELP
	OrbitEcli orbit_moon = moon.orbitAlfaELP( JD );
	Grade lambda_moon = orbit_moon.L;
	Grade beta_moon = orbit_moon.B;

	OrbitEqu   o_moon ( JD );
	// converte le coordinate eclittiche in equatoriali
	o_moon = orbit_moon;

	Grade alfa_moon = o_moon.alfa;
	Grade delta_moon = o_moon.delta; // declinazione


	// converte in coordinate rettangolari
	OrbitXYZ o_moon_xyz = o_moon;
	o_moon_xyz.rot_x += 1.543 ;

	moon.position = o_moon_xyz ;
	// orientamento della luna rispetto alla terra
	// devo ruotare esattamente di alfa_moon
	//		Grade rot_luna = orbit_moon.lambda;
	// TODO non funziona ancora
	Grade rot_luna =  180 + alfa_moon;
	o_moon_xyz.rot_y = rot_luna ;

	/*
	l'orbita dei pianeti è calcolata con VSOP C (rettangolare ecclittica eliocentrica)
	* convertita ad equatoriale geocentrica
	utilizzando l'orbita rettangolare del sole e eccentricità terra
	*/

	OrbitXYZ orbit_sun_xyz = sun.orbit ( MainBody::earth_data, JD ) ;


	Radiant eps = epsilon;


	Mercury & mercury  = data->system->mercury  ;
	OrbitXYZ orbit_mercury_xyz = mercury.orbit_eq ( MainBody::mercury_data, orbit_sun_xyz, epsilon, JD ) ;

	mercury.position = orbit_mercury_xyz ;

	Venus & venus  = data->system->venus  ;
	OrbitXYZ orbit_venus_xyz = venus.orbit_eq ( MainBody::venus_data, orbit_sun_xyz, epsilon, JD ) ;

	venus.position = orbit_venus_xyz ;

	Mars & mars  = data->system->mars  ;

	OrbitXYZ orbit_mars_xyz = mars.orbit_eq ( MainBody::mars_data, orbit_sun_xyz, epsilon, JD ) ;

	mars.position = orbit_mars_xyz ;

	Jupiter & jupiter  = data->system->jupiter  ;
	OrbitXYZ orbit_jupiter_xyz = jupiter.orbit_eq ( MainBody::jupiter_data, orbit_sun_xyz, epsilon, JD ) ;

	double x_jupiter = orbit_jupiter_xyz.x ;
	double y_jupiter = orbit_jupiter_xyz.y ;
	double z_jupiter = orbit_jupiter_xyz.z ;
	jupiter.position = orbit_jupiter_xyz ;
	orbit_jupiter_xyz.rot_x += jupiter.obliquity ( JD ) ;

	Io & io = data->system->io ;
	OrbitXYZ orbit_io_xyz = io.orbit ( JD ) ;
	orbit_io_xyz.x += x_jupiter ;
	orbit_io_xyz.y += y_jupiter ;
	orbit_io_xyz.z += z_jupiter ;

	// orientamento di io rispetto jupiter
	// devo ruotare di atan(y/x)

	io.position = orbit_io_xyz ;

	Europa & europa = data->system->europa ;
	OrbitXYZ orbit_europa_xyz = europa.orbit ( JD ) ;

	orbit_europa_xyz.x += x_jupiter ;
	orbit_europa_xyz.y += y_jupiter ;
	orbit_europa_xyz.z += z_jupiter ;
	europa.position = orbit_europa_xyz ;

	Ganimede & ganimede = data->system->ganimede ;
	OrbitXYZ orbit_ganimede_xyz = ganimede.orbit ( JD ) ;
	orbit_ganimede_xyz.x += x_jupiter ;
	orbit_ganimede_xyz.y += y_jupiter ;
	orbit_ganimede_xyz.z += z_jupiter ;
	ganimede.position = orbit_ganimede_xyz ;

	Callisto & callisto = data->system->callisto ;
	OrbitXYZ orbit_callisto_xyz = callisto.orbit ( JD ) ;
	orbit_callisto_xyz.x += x_jupiter ;
	orbit_callisto_xyz.y += y_jupiter ;
	orbit_callisto_xyz.z += z_jupiter ;
	callisto.position = orbit_callisto_xyz ;

	Saturn & saturn = data->system->saturn ;
	OrbitXYZ orbit_saturn_xyz = saturn.orbit_eq ( MainBody::saturn_data, orbit_sun_xyz, epsilon, JD ) ;

	saturn.position = orbit_saturn_xyz ;

	Uranus & uranus = data->system->uranus ;
	OrbitXYZ orbit_uranus_xyz = uranus.orbit_eq ( MainBody::uranus_data, orbit_sun_xyz, epsilon, JD ) ;
	uranus.position = orbit_uranus_xyz ;

	Neptune & neptune = data->system->neptune ;
	OrbitXYZ orbit_neptune_xyz = neptune.orbit_eq ( MainBody::neptune_data, orbit_sun_xyz, epsilon, JD ) ;

	neptune.position = orbit_neptune_xyz ;



	std::stringstream e_fov, e_zoom, e_near, e_far ;
	if ( data->camera.fov != data->camera.old_fov )
		e_fov << "		<fov value=\"" << data->camera.fov << "\" />" ;
	if ( data->camera.zoom != data->camera.old_zoom )
		e_zoom << "		<zoom value=\"" << data->camera.zoom << "\" />" ;
	if ( data->camera.near != data->camera.old_near )
		e_near << "		<near value=\"" << data->camera.near << "\" />" ;
	if ( data->camera.far != data->camera.old_far )
		e_far << "		<far value=\"" << data->camera.far << "\" />" ;

		// il punto di osservazione
	clientData::Camera &camera = data->camera;
	double lookat_x = camera.lookat_x;
	double lookat_y = camera.lookat_y;
	double lookat_z = camera.lookat_z;

	double pos_x = camera.pos_x;
	double pos_y = camera.pos_y;
	double pos_z = camera.pos_z;

	double up_x = camera.up_x;
	double up_y = camera.up_y;
	double up_z = camera.up_z;
/*
	// il punto di osservazione
	clientData::Camera &camera = data->camera;
	double lookat_x = 0;
	double lookat_y = 0;
	double lookat_z = 0;

	double pos_x = 1;
	double pos_y = 1;
	double pos_z = 1;

	double up_x = 0;
	double up_y = 0;
	double up_z = 1;

	camera.lookat_x = lookat_x;
	camera.lookat_y = lookat_y;
	camera.lookat_z = lookat_z;

	camera.pos_x = pos_x;
	camera.pos_y = pos_y;
	camera.pos_z = pos_z;
*/
	double FACT = data->FACT;


	OrbitXYZ sky_pos ( 0, 0, 0 );
	data->system->sky.position = sky_pos;

	SolarSystem * system = data->system;
	std::string primitives = system->primitive();

	Sky & sky = data->system->sky ;
	std::string map = sky.map();

	std::string sky_point ;

	if ( map.substr(0,4) == "mag:" ) {

		float mag = atof(map.substr(4).c_str());
		SkyGrid & skyGrid = *data->skyGrid;
		sky_point = skyGrid.createSkyStars( mag );

	}

	std::stringstream xml;

	xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<

			"<results>" <<

				"<renderer name=\"solar\" color=\"0x000000\" shadow=\"shadow\"" << " height=\"700\">" <<

				"	<camera type=\"perspective\">" <<
					"	<position x=\"" << FACT * pos_x << "\" y=\"" << FACT * pos_y << "\" z=\"" << FACT * pos_z << "\" />"	<<
				"		<lookat x=\"" << FACT * lookat_x << "\" y=\"" << FACT * lookat_y << "\" z=\"" << FACT * lookat_z << "\" />" <<
				"		<up x=\"" << up_x << "\" y=\"" << up_y << "\" z=\"" << up_z << "\" />" <<

						e_zoom.str() <<
						e_fov.str() <<
						e_near.str() <<
						e_far.str() <<

				"		<shadow />" <<
				"	</camera>" <<

					"<light  type=\"point\" color=\"0xffffff\" intensity=\"1.0\" name=\"sunlight\">" <<
					"	<shadow />" <<
					"	<position x=\"" << FACT * x_sun << "\" y=\"" << FACT * y_sun << "\" z=\"" << FACT * z_sun << "\" />"	<<
					"</light>" <<

					primitives <<

					sky_point <<

				"</renderer>" <<

			"</results>" ;

		// manda i dati al client
//		return utils::compressToWebSocket(xml.str());
		return xml.str();


}	// std::string WSsrv::render ( int id ) {



void WSsrv::print_log ( const std::string & log_str ) const
{

	std::string t = timestamp(  );

    std::cout <<  t << " " << log_str << std::endl ;

}


long WSsrv::sendRFC6455 ( int /* fd */, const std::string & /* type */, const std::string & /* message */, bool /* masked */ ) const {



	return 0;

}

/* This function return a timestamp string
 */
std::string WSsrv::timestamp( ) const {

	time_t now;
	struct tm *time_struct;
	char time_buffer[40];

	time( &now );  // get number of seconds since epoch
	time_struct = localtime( (const time_t *) & now ); // convert to tm struct

	strftime( time_buffer, 40, "%d/%m/%Y %H:%M:%S> ", time_struct );

	return time_buffer;

}

void WSsrv::unregisterClient ( int id ) {
	auto it = clients.find ( id );
	if ( it != clients.end() )
		clients.erase (id);
}


std::string WSsrv::registerClient ( int id, const std::string &query  ) {


	// verifica se ci sia un altro client con lo stesso id, nel caso lo cancella
	auto it = clients.find ( id );
	if ( it != clients.end() )
		clients.erase (id);

	Client * client = new Client ;
	clients.insert ( std::pair< int, Client *> ( id, client ) ) ;

	client->socket = id;
	client->setQueryString ( query ) ;


//	bool ret = true ;

	std::stringstream ss ;

	JSON json;
	json.loadFromString(query);

	int sockfd = client->socket;
	std::string user = json["user"] ;
	std::string file = json["file"] ;
	std::string document_root = json["document_root"] ;
	std::string system_data = json["system_data"];

	ss << "root =" << document_root << " file=" << file << " system=" << system_data;

	SolarSystem * system  ;
	try {

		system = new SolarSystem ( system_data ) ;

	} catch ( const std::string & str ) {

		print_log( str );

	}

	clientData * data = new clientData ( this, system, sockfd );

	// registra i dati extra per il client
	client->extra = data ;


	try {

		data->asterismi = data->skyGrid->trackAsterism ();

	} catch ( const std::string & str ) {

		print_log( str );

	}


		try {
      std::string ecliptic = data->skyGrid->ecliptic( 2024 );



      std::stringstream xml;
      xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<
        "<results>" <<
        "	<renderer>" <<
//        ecliptic <<
        "	</renderer>"  <<
          "<data>" <<
            "<span value=\"" << data->camera.fov << "\" id=\"span_fov\" />" <<
            "<ui value=\"" << data->camera.fov << "\" id=\"fov\" type=\"slider\" />" <<
            "<span value=\"" << data->camera.zoom << "\" id=\"span_zoom\" />" <<
            "<ui value=\"" << 10 * data->camera.zoom << "\" id=\"zoom\" type=\"slider\" />" <<
            "<span value=\"" << data->simulation << "\" id=\"span_simul\" />" <<
            "<ui value=\"" << data->simulation << "\" id=\"simul\" type=\"slider\" />" <<
          "</data>" <<
        "</results>" ;

     
      return xml.str();

		}
		catch ( const std::string & ss ) {

			std::cout << ss << std::endl;
			std::cout << "**+++" << std::endl;

		}
    catch ( ... ) {

          std::cerr << "Catturata un'eccezione sconosciuta o generica." << std::endl;
    }

}


class clientData  * WSsrv::client_data ( int sockfd )
{
  auto it = clients.find ( sockfd );
  Client * client = it->second;

  return reinterpret_cast < clientData * > ( client->extra );
}



std::string  WSsrv::executeLoop ( int id  )
{

	
  
  auto it = clients.find ( id );
	Client * client = it->second;

	clientData * data = reinterpret_cast < clientData * > ( client->extra );

	if ( !data ) return "";

	
  std::string xml = do_sendLoop( id ) ;
		
		std::stringstream render;
    render <<
      "{" << 
        "\"pagina\" : \"planetario\"," <<
        "\"azione\" : \"render\"," <<
        "\"xml\" : \"" << utils::compressToWebSocket(xml) << "\"" <<
      "}";
    
    return render.str();

}	// std::string  WSsrv::executeLoop ( int id  )

std::string WSsrv::setLight ( int id ) {

	auto it = clients.find ( id );
	Client * client = it->second;

	clientData * data = reinterpret_cast < clientData * > ( client->extra );

  
	Sun & sun = data->system->sun ;

  std::stringstream jLight;
  
	double FACT = data->FACT;
  jLight << 
    "{" <<
      "tipo:\"point\"," <<
      "color:\"0xffffff\"," <<
      "intensity:\"1.0\"," <<
      "name:\"sunLight\"," <<
      "position:{" <<
        "x:" << FACT * sun.position.x << "," <<
        "y:" << FACT * sun.position.y << "," <<
        "z:" << FACT * sun.position.z << 
      "}" <<
    "}" ;
    
    return jLight.str();

}

std::string WSsrv::setCamera ( int id ) {

	auto it = clients.find ( id );
	Client * client = it->second;

	clientData * data = reinterpret_cast < clientData * > ( client->extra );

  
	Sun & sun = data->system->sun ;
	Earth & earth = data->system->earth ;
	Moon & moon = data->system->moon ;
	Mars & mars = data->system->mars ;

  // imposta camera.lookat
	set_horizont( data );

	// il punto di osservazione
	clientData::Camera &camera = data->camera;
	double lookat_x = camera.lookat_x;
	double lookat_y = camera.lookat_y;
	double lookat_z = camera.lookat_z;

	double pos_x = camera.pos_x;
	double pos_y = camera.pos_y;
	double pos_z = camera.pos_z;

	double up_x = camera.up_x;
	double up_y = camera.up_y;
	double up_z = camera.up_z;

	double latitude = data->lookfrom.latitude;
	double longitude = data->lookfrom.longitude;
  double rot = data->rotation;
	Horizont horizont ( latitude, longitude + rot );

		std::stringstream track_horizont ;
		std::string track = horizont.track ( data->horizont );

		track_horizont << track ;

		if ( data->lookfrom.isFromPlace ) {


			std::stringstream xml;

			if ( data->lookfrom.where == "earth" ) {


				Point pdv = horizont.pointOfView();

				pos_x = pdv.x;
				pos_y = pdv.y;
				pos_z = pdv.z;

				camera.up_x = pos_x;
				camera.up_y = pos_y;
				camera.up_z = pos_z;

			}
			else if ( data->lookfrom.where == "moon" ) {

				double rot_luna = moon.position.rot_y;
        Point coord = moon.bodyCoord ( latitude, longitude + rot_luna );

				pos_x = moon.position.x + coord.x / CelestialBody::AU;
				pos_y = moon.position.y + coord.y / CelestialBody::AU;
				pos_z = moon.position.z + coord.z / CelestialBody::AU;

			}
			else if ( data->lookfrom.where == "mars" ) {

				Point coord = mars.bodyCoord ( latitude, longitude );

				pos_x = mars.position.x + coord.x / CelestialBody::AU;
				pos_y = mars.position.y + coord.y / CelestialBody::AU;
				pos_z = mars.position.x + coord.z / CelestialBody::AU;

			}
			else if ( data->lookfrom.where == "sun" ) {

				Point coord = sun.bodyCoord ( latitude, longitude );

				pos_x = sun.position.x + coord.x / CelestialBody::AU;
				pos_y = sun.position.y + coord.y / CelestialBody::AU;
				pos_z = sun.position.z + coord.z / CelestialBody::AU;

			}
			else {

				auto it = data->system->bodies.find( data->lookfrom.where );
				if ( it != data->system->bodies.end() ) {

					CelestialBody & body = it->second;
					Point coord = body.bodyCoord ( latitude, longitude );

					pos_x = body.position.x + coord.x / CelestialBody::AU;
					pos_y = body.position.y + coord.y / CelestialBody::AU;
					pos_z = body.position.z + coord.z / CelestialBody::AU;

				}


			}


		}

//		double scale = data->scale;

		if ( data->lookat.where == "earth" ) {

			if ( !data->lookfrom.isFromPlace ) {

				// se non osservi da un punto particolare allora gurda dal sole
				Point p1 (sun.position.x, sun.position.y, sun.position.z);
				Point p2 ( 0, 0, 0 );

				double r = p1.distance( p2 );

				StraightLine line ( p1, p2);

				Point p = line.pointAtDistance( r * 0.995 );

				pos_x = p.x ;
				pos_y = p.y ;
				pos_z = p.z ;

			}

			lookat_x = 0;
			lookat_y = 0;
			lookat_z = 0;

		}
		else if ( data->lookat.where == "earth moon" ) {


				Point p1 ( moon.position.x, moon.position.y, moon.position.z );
				Point p2 ( 0, 0, 0 );

				double r = p2.distance( p1 );

				StraightLine line ( p2, p1);

        double r_moon = moon.radius / CelestialBody::AU ;
				Point p = line.pointAtDistance( r - r_moon  );

			pos_x = p.x  ;
			pos_y = p.y ;
			pos_z = p.z ;

			lookat_x = 0;
			lookat_y = 0;
			lookat_z = 0;

		}
		else if ( data->lookat.where == "sky" ) {

			if ( !data->lookfrom.isFromPlace ) {

				// se non osservi da un punto particolare allora guarda dalla terra
				/*
mi posizione sulla crosta terrestre nella linea che
unisce il centro della terra con il corpo celesto
moltiplico per 1000 in quanto i valori sono troppo piccoli
e potrebbe convergere a 0
				 */
				Point p1 ( 0, 0, 0 );
				Point p2 ( 1000 * moon.position.x, 1000 * moon.position.y, 1000 * moon.position.z );

				double r = p2.distance( p1 );

				StraightLine line ( p2, p1);

				double r_earth = earth.radius / CelestialBody::AU ;
        Point p = line.pointAtDistance( r - 1000 * r_earth  );

				pos_x = p.x / 1000 ;
				pos_y = p.y / 1000 ;
				pos_z = p.z / 1000 ;

			}

		}
		else if ( data->lookat.where == "sun" ) {

			lookat_x = sun.position.x ;
			lookat_y = sun.position.y ;
			lookat_z = sun.position.z ;

		}
		else {

			auto it = data->system->bodies.find( data->lookat.where );
			if ( it != data->system->bodies.end() ) {

				CelestialBody & body = it->second;


				if ( !data->lookfrom.isFromPlace ) {

// se non osservi da un punto particolare allora guarda dalla terra
				/*
mi posizione sulla crosta terrestre nella linea che
unisce il centro della terra con il corpo celesto
moltiplico per 1000 in quanto i valori sono troppo piccoli
e potrebbe convergere a 0
				 */
					double fact = 100 ;
					double r_body = body.radius / CelestialBody::AU;;
					Point p1 ( 0, 0, 0 );
					Point p2 ( fact * body.position.x, fact * body.position.y, fact * body.position.z );

					double r = p1.distance( p2 );

					StraightLine line ( p1, p2);

					Point p = line.pointAtDistance( r - fact * 100 * r_body  );

					pos_x = p.x / fact ;
					pos_y = p.y / fact ;
					pos_z = p.z / fact ;


				}

				lookat_x = body.position.x ;
				lookat_y = body.position.y ;
				lookat_z = body.position.z ;

			}

	}


	data->camera.old_fov = data->camera.fov;
	data->camera.old_zoom = data->camera.zoom;
	data->camera.old_far = data->camera.far;
	data->camera.old_near = data->camera.near;


	camera.lookat_x = lookat_x;
	camera.lookat_y = lookat_y;
	camera.lookat_z = lookat_z;

	camera.pos_x = pos_x;
	camera.pos_y = pos_y;
	camera.pos_z = pos_z;
    
  std::stringstream camera_json ;
    
	double FACT = data->FACT;
  camera_json 
      << "{" <<
        "position:{" <<
          "x:" << FACT * pos_x << "," <<
          "y:" << FACT * pos_y << "," <<
          "z:" << FACT * pos_z << 
        "}," <<
        "lookat:{" <<
          "x:" << FACT * pos_x << "," <<
          "y:" << FACT * pos_y << "," <<
          "z:" << FACT * pos_z <<
        "}," <<
        "up:{" <<
          "x:" << FACT * pos_x << "," <<
          "y:" << FACT * pos_y << "," <<
          "z:" << FACT * pos_z <<
        "}," <<
        "zoom:" << data->camera.zoom << "," <<
        "fov:" << data->camera.fov << "," <<
        "near:" << data->camera.near << "," <<
        "far:" << data->camera.far << "," <<
        "shadow:" << "true" << 
    "}";
     
   return camera_json.str();

}

clientData::clientData (  WSsrv * p, SolarSystem * _system, int socked_id ) :
	parent (p),
	system (_system),
	sockfd(socked_id),
	timer( * new Timer() ),
	timer_result( * new Timer() ),
	ref_date ( * new Date() )
{

	skyGrid = new SkyGrid(this);

	timer.stop ();
	timer_result.stop ();

	set_default();

}


clientData::~clientData () {

	delete skyGrid ;
	delete system ;
	delete & timer;
	delete & timer_result;
	delete & ref_date;
}
