#ifndef _celestial_body_
#define _celestial_body_

#ifndef _cmat_
   #include <cmat.hpp>
#endif

#ifndef _variable_
   #include <variabile.hpp>
#endif

#ifndef _funzione_
   #include <funzione.hpp>
#endif


#include <math.h>       /* pow */

#include <fstream>
#include <iostream>
#include <map>
#include <vector>
#include <vsop.hpp>
#include <elp.hpp>
#include <angolo.hpp>
#include <date.hpp>
#include <point.hpp>


class CelestialBody  {

public :

	static const std::string DATADIR ;
	static const double pi ;
	static const double G ;
	static const double LIGHT_SPEED  ; // km / s
	static const double AU ; // km

	double a  ;	// raggio all'equatore km
	double b  ;	// raggio al polo km
	double radius ;

	double M ;	// massa in kg
	double D;   // diametro km
	double R;   // raggio in m
	

	struct {

		std::string shader;
		std::string map ;
		std::string side = "front" ;
		std::string shadow ;
		double scale = 1;
		double FACT = 1;
		
		double widthSegments = 30;
		double heightSegments = 30;
		
	} gl ;
	
protected:

	friend class SolarSystem;

	double sin_deg ( Grade g );
	double cos_deg ( Grade g );
	
	CelestialBody * parent ;
	std::vector< CelestialBody *> childs ;

	std::string name ;
    
	double size_fact = 1;	// fattore di scala diametro

public:

	OrbitXYZ position ;
	double obliq ;	// grade
	

	CelestialBody ( ) :
		parent ( nullptr ), name ( "" ) {}
		
	CelestialBody ( const std::string & data, CelestialBody * parent ) ;
	virtual ~CelestialBody() ;

//	bool manageBody ( const	XMLElement * el ) ;

	virtual OrbitXYZ orbit ( double /* T */ ) const {
		return OrbitXYZ();
		}

	std::string showOrbitalData ( double T ) ;
	std::string primitive ( double T ) const ;
  std::string primitiveJSON ( double T ) const ; 

	std::string showOrbitalData ( OrbitXYZ orbit ) ;

	virtual std::string primitive ( OrbitXYZ & orbit, bool recreate = false ) const ;
	virtual std::string primitive ( bool recreate = false ) const ;
	
	virtual std::string primitive_pos ( OrbitXYZ & orbit ) const ;
	std::string primitive_pos () const ;

	Point bodyCoord ( double latitude, double longitude, double H = 0 ) const ;
	
	void set_size_fact ( double fact ) {
		size_fact = fact;
	}

	double get_size_fact ( ) const {
		return size_fact ;
	}

	double get_radius ( ) const {
		return radius ;
	}
	
	virtual void setMap ( const std::string & directory, const std::string & map ) ;

	std::string map () const {
		return gl.map;
	}

};

class Sky : public CelestialBody {

public:

	Sky ( ) ;
	~Sky() ;

	std::string color = "0x000000";
	
	virtual std::string primitive ( bool recreate = false ) {
		return primitive( position, recreate );
		}
	virtual std::string primitive ( OrbitXYZ & orbit, bool recreate = false ) const ;
	virtual void setMap ( const std::string & directory, const std::string & map ) ;

};

class MainBody : public CelestialBody  {

protected:

	const VSOP * vsop;

public:

  using CelestialBody::orbit; // <-- mantiene visibili tutti gli overload della base
	
  static const VSOP venus_data;
	static const VSOP venus_LBR_D_data;
	
	static const VSOP mercury_data;
	static const VSOP mercury_LBR_D_data;

	static const VSOP uranus_data;
	static const VSOP uranus_LBR_D_data;

	static const VSOP saturn_data;
	static const VSOP saturn_LBR_D_data;

	static const VSOP neptune_data;
	static const VSOP neptune_LBR_D_data;

	static const VSOP mars_data;
	static const VSOP mars_LBR_D_data;

	static const VSOP jupiter_data;
	static const VSOP jupiter_LBR_D_data;

	static const VSOP earth_data;
	static const VSOP earth_LBR_A_data;
	static const VSOP earth_LBR_B_data;
	static const VSOP earth_LBR_D_data;
	


	MainBody ( ) :
		CelestialBody ( ), vsop( nullptr ) {}
		
	MainBody ( const std::string & name, CelestialBody * parent, const class VSOP * ) ;
	~MainBody() ;

	OrbitXYZ orbit_eq ( const VSOP & vsop, const OrbitXYZ sun_pos, double epsilon, double JD ) const ;
	OrbitXYZ orbit_eq ( const VSOP & vsop, double JD ) const ;
	OrbitXYZ orbit ( const VSOP & vsop, double JD ) const ;
	OrbitXYZ orbit ( double JD ) const ;
	virtual OrbitEcli orbitLBR ( double JD ) const ;

};

class Earth : public MainBody  {


	static const short n22A = 63;
    static const double tab22A [n22A][9] ;

	double calculateEquiSosti ( double JDE0, double long_degree );

public:

	
	Earth ( ) ;
	Earth ( CelestialBody * parent ) ;
	Earth ( const class VSOP *  ) ;

	~Earth() ;

	virtual std::string primitive_pos ( OrbitXYZ & orbit ) const ;

	struct Nutation {

		double Dpsi = 0;
		double Depsilon = 0;

	};

    struct Precession {

        double alfa = 0;
        double delta = 0;
        
        Precession ( double a, double d ) :
			alfa( a) , delta( d ) {
			}

    };

	
	double sunAngleOverOrizont  ( double JD, double latitude, double longitude ) const ;
	double sunRise  ( const Date & d, double lat, double lng, double simulation ) const ;
	double rotation ( const Date &, double simulation = 1 ) const ;
	double rotation ( double JD ) const ;
	
	double equinox ( short year, short mounth );

	double marchEquinox ( short year );
	double juneSolstice ( short year );
	double septemberEquinox ( short year );
	double decemberSolstice ( short year );

	std::string Easter ( short year ) ;
	std::string EasterEcle ( short year ) ;

	Nutation nutation ( double JD );
	double obliquity ( double JD );
	Precession precession ( double latitude, double longitude, double JD );

};


class Sun  : public MainBody {

	public :
	
	Sun ( ) ;
	~Sun () {}

	OrbitEcli orbitLBR ( double JD ) const ;
	
};

class Venus  : public MainBody {

	public :
	
	Venus ( ) ;
	~Venus () {}

};

class Mars  : public MainBody {

	public :
	
	Mars ( ) ;
	~Mars () {}

};

class Jupiter  : public MainBody {

	public :
	
	Jupiter ( ) ;
	~Jupiter () {}

	double obliquity ( double JD );

};

class Saturn  : public MainBody {

	public :
	
		double ringInnerR;
		double ringOuterR;
	
		virtual std::string primitive ( OrbitXYZ & orbit, bool recreate = false ) const ;
		
		Saturn ( ) ;
		~Saturn () {}

};

class Uranus  : public MainBody {

	public :
	
		double ringInnerR;
		double ringOuterR;
	
		virtual std::string primitive ( OrbitXYZ & orbit, bool recreate = false ) const ;
		
		Uranus ( ) ;
		~Uranus () {}

};

class Neptune  : public MainBody {

	public :
	

		Neptune ( ) ;
		~Neptune () {}

};

class Mercury  : public MainBody {

	public :
	
	Mercury ( ) ;
	~Mercury () {}

};

class Moon : public CelestialBody  {

	static const short n47 = 60;

	static const double tab47A [n47][6];

	static const double tab47B [n47][5];
    static const double tab47B_copia [n47][5];

    static ELP_GROUP1 elp1;
    static ELP_GROUP1 elp2 ;
    static ELP_GROUP1 elp3 ;

    static ELP_GROUP2 elp4 ;
    static ELP_GROUP2 elp5 ;
    static ELP_GROUP2 elp6 ;

    static ELP_GROUP2 elp7 ;
    static ELP_GROUP2 elp8 ;
    static ELP_GROUP2 elp9 ;

    static ELP_GROUP3 elp10 ;
    static ELP_GROUP3 elp11 ;
    static ELP_GROUP3 elp12 ;


    static ELP_GROUP3 elp13 ;
    static ELP_GROUP3 elp14 ;
    static ELP_GROUP3 elp15 ;

    static ELP_GROUP4 elp16 ;
    static ELP_GROUP4 elp17 ;
    static ELP_GROUP4 elp18 ;

    static ELP_GROUP4 elp19 ;
    static ELP_GROUP4 elp20 ;
    static ELP_GROUP4 elp21 ;

    static ELP_GROUP5 elp22 ;
    static ELP_GROUP5 elp23 ;
    static ELP_GROUP5 elp24 ;

    static ELP_GROUP5 elp25 ;
    static ELP_GROUP5 elp26 ;
    static ELP_GROUP5 elp27 ;

    static ELP_GROUP5 elp28 ;
    static ELP_GROUP5 elp29 ;
    static ELP_GROUP5 elp30 ;

    static ELP_GROUP5 elp31 ;
    static ELP_GROUP5 elp32 ;
    static ELP_GROUP5 elp33 ;

    static ELP_GROUP5 elp34 ;
    static ELP_GROUP5 elp35 ;
    static ELP_GROUP5 elp36 ;

	struct PeriodicTerm {
		double longitude, distance;
		PeriodicTerm ( double l, double d ) :
			longitude( l ), distance ( d ) {}
	};

	PeriodicTerm periodicTerm ( double D, double M, double M1, double F, double E);
	double latitude ( double D, double M, double M1, double F, double E);

public:

	Moon (  ) ;
	~Moon() ;

  using CelestialBody::orbit; // <-- mantiene visibili tutti gli overload della base

	virtual std::string primitive_pos ( OrbitXYZ & orbit ) const ;

	OrbitXYZ orbit ( double JD ) ;
	OrbitEcli orbitAlfa ( double JD ) ;
	OrbitEcli orbitAlfaELP ( double JD ) ;
	std::string showOrbitalAlfaData ( double JD ) ;

	// calcola la fase lunare più prossima a JD
	double phases ( double JD, double phase );
	std::vector< double > nodes ( short year );

	double angleME ( double JD );
	double angleME ( double JD, Earth & earth ) ;

};

struct FLongitudine : public Funzione {

		double grade = 0;
		Variabile JD;

		double eval () {

			Sun sun;
			OrbitEcli orbit = sun.orbitLBR ( JD ) ;

            double L = orbit.L;
            Grade B = orbit.B;
            double R = orbit.R;

            Earth earth;
            Earth::Nutation n = earth.nutation( JD );

            double FK5 = - 0.09033 / 3600;
            double aberrazione = - ( 20.4898 / R ) / 3600;
 
            // facciamo il complemento
            if ( L < 0 ) L += 360;
            double lambda = L + n.Dpsi + FK5 + aberrazione;

			return lambda - grade ;

		}

		FLongitudine ( double _grade ) :
            grade ( _grade ) {
		}

} ;

struct FDeclination : public Funzione {

		double grade = 0;
		Variabile JD;

		double eval () {

			Sun sun;
			OrbitEcli orbit_sun = sun.orbitLBR( JD );	// coordinate eclittiche


			OrbitEqu   o_sun ( JD );
			o_sun = orbit_sun;	// converte in coordinate equatoriali

//			double ar = o_sun.alfa  ;

/*
            Earth earth;
            Earth::Nutation n = earth.nutation( JD );
			double De =  n.Depsilon;
* */
			double dec = o_sun.delta ;

			return dec - grade ;

		}

		FDeclination ( double _grade ) :
            grade ( _grade ) {
		}

} ;

#endif	// if _celestial_body_
