#ifndef _elp_
#define _elp_

#include <vector>
#include <angolo.hpp>

/*
*     -----
*     Files
*     -----
*     The program reads the ELP2000-82B files.
*     Organization : 36 sequential ascii files.
*     Record length : 100 bytes.
*     Names : ELP82xx (xx = 1 to 36).
*     Longitude files : 1, 4, 10, 16, 22, 28, 31, 7, 13, 19, 25, 34.
*     Latitude  files : 2, 5, 11, 17, 23, 29, 32, 8, 14, 20, 26, 35.
*     Distance  files : 3, 6, 12, 18, 24, 30, 33, 9, 15, 21, 27, 36.

*   GROUP 1 : ELP1 to ELP3
*   GROUP 2 : ELP4 to ELP9
*   GROUP 3 : ELP10 to ELP15
*   GROUP 4 : ELP16 to ELP21
*   GROUP 5 : ELP22 to ELP36
*/

class ELP {


    protected :

        const double pi = atan( 1 ) * 4 ;

        std::string header;
        std::string file_name;

	public:

		ELP ( const std::string & nome_file );
		virtual double sumSin ( double T )   const = 0;
		virtual double sumCos ( double T )   const = 0;

		double Longitude ( double T ) {
            return sumSin ( T ) ;
		}

		double Latitude ( double T )  const {
            return sumSin ( T ) ;
		}

		double Distance ( double T ) const {
            return sumCos ( T ) ;
		}


};

class ELP_GROUP1 : public ELP {


    protected :

        struct Data {
            short i1, i2, i3, i4 ;
            double A;
        };

        std::vector< Data > data;


	public:

		ELP_GROUP1 ( const std::string & nome_file );
		double sumSin ( double T )   const ;
		double sumCos ( double T )   const ;


};

class ELP_GROUP2 : public ELP {


    protected :

        struct Data {
            short i1, i2, i3, i4, i5 ;
            double PHI, A;
        };

        std::vector< Data > data;


	public:

		ELP_GROUP2 ( const std::string & nome_file );
		double sumSin ( double T )   const ;
		double sumCos ( double T )   const ;


};


class ELP_GROUP3 : public ELP {


    protected :

        struct Data {
            short i1, i2, i3, i4, i5, i6, i7, i8, i9, i10, i11 ;
            double PHI, A;
        };

        std::vector< Data > data;


	public:

		ELP_GROUP3 ( const std::string & nome_file );
		double sumSin ( double T )   const ;
		double sumCos ( double T )   const ;


};

class ELP_GROUP4 : public ELP_GROUP3 {


    protected :

	public:

		ELP_GROUP4 ( const std::string & nome_file );
		double sumSin ( double T )   const ;
		double sumCos ( double T )   const ;


};

class ELP_GROUP5 : public ELP_GROUP2 {


    protected :

	public:

		ELP_GROUP5 ( const std::string & nome_file );


};

struct Delaunay {

    static const double p  ;  // general precession jd2000 mas x century

	double TDB;

	double D;
	double l1;
	double l;
	double F;
	double Chi;

	virtual double W1 ( ) = 0;
	virtual double W2 ( ) = 0 ;
	virtual double W3 ( ) = 0 ;
	virtual double T ( ) = 0 ;
	virtual double Omega1 ( ) = 0 ;

	Delaunay ( double T ) ;

	void Init ();

};

struct DelaunayComplete : public Delaunay {

	double W1 ( ) ;
	double W2 ( )  ;
	double W3 ( )  ;
	double T ( )  ;
	double Omega1 ( )  ;
	double pA ( )  ;

	DelaunayComplete ( double T ) ;

};

struct DelaunayLinear  : public Delaunay {

	double W1 ( ) ;
	double W2 ( )  ;
	double W3 ( )  ;
	double T ( )  ;
	double Omega1 ( )  ;

	DelaunayLinear ( double T ) ;

};

struct PlanetaryLongitudes {


	double Me  ;
	double V   ;
	double T   ;
	double Ma   ;
	double J   ;
	double S   ;
	double U   ;
	double N   ;


	PlanetaryLongitudes ( double T ) ;

};


#endif


