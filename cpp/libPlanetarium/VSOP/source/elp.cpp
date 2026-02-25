#include <fstream>
#include <sstream>
#include <math.h>       /* cos sin */

#include <elp.hpp>


ELP::ELP ( const std::string & nome_file ) :
    file_name( nome_file )
{
}

ELP_GROUP1::ELP_GROUP1 ( const std::string & nome_file ) :
    ELP( nome_file )
{

	std::ifstream ifs ( file_name.c_str(), std::ifstream::in );

  	if ( !ifs.good() ) {

		std::stringstream ss ;
		ss << " good()=" << ifs.good() << std::endl;
		ss << " eof()=" << ifs.eof() << std::endl;
		ss << " fail()=" << ifs.fail() << std::endl;
		ss << " bad()=" << ifs.bad() << std::endl;
		ss << " errore " << file_name.c_str() << std::endl;

		ifs.close();

	 	throw ( ss.str() ) ;

	}

    // la prima riga è l'header
    std::getline( ifs, header ) ;

  	while ( !ifs.eof () ) {

		std::string str;
		std::stringstream ss ;

		std::getline( ifs, str ) ;
		ss.precision(10);
		ss << str ;

        Data d ;
		ss.precision(10);
		ss >> d.i1 >> d.i2 >> d.i3 >> d.i4 >> d.A ;

        data.push_back( d );

	}

	ifs.close();

}

double ELP_GROUP1::sumSin ( double T )  const
{

    DelaunayComplete d ( T );

	double D     = d.D;
	double l1    = d.l1;
	double l     = d.l;
	double F     = d.F ;

	double sum = 0 ;
	for ( size_t i = 0; i < data.size(); ++ i) {

        short i1 = data[i].i1;
        short i2 = data[i].i2;
        short i3 = data[i].i3;
        short i4 = data[i].i4;

        double A = data[i].A;

        Radiant arg = Grade ( i1 * D + i2 * l1 + i3 * l + i4 * F ) ;

        sum += A * sin( arg );

	}

    return sum ;

}

double ELP_GROUP1::sumCos ( double T )  const
{

    DelaunayComplete d ( T );

	double D     = d.D;
	double l1    = d.l1;
	double l     = d.l;
	double F     = d.F ;

	double sum = 0;
	for ( size_t i = 0; i < data.size(); ++ i) {

        short i1 = data[i].i1;
        short i2 = data[i].i2;
        short i3 = data[i].i3;
        short i4 = data[i].i4;

        double A = data[i].A;

        Radiant arg = Grade ( i1 * D + i2 * l1 + i3 * l + i4 * F ) ;

        sum += A * cos( arg );

	}

    return sum ;

}

// group 2

ELP_GROUP2::ELP_GROUP2 ( const std::string & nome_file ) :
    ELP( nome_file )
{

	std::ifstream ifs ( file_name.c_str(), std::ifstream::in );

  	if ( !ifs.good() ) {

		std::cout << " good()=" << ifs.good();
		std::cout << " eof()=" << ifs.eof();
		std::cout << " fail()=" << ifs.fail();
		std::cout << " bad()=" << ifs.bad();
		std::cout << " errore " << file_name.c_str() << std::endl;

		ifs.close();

		std::stringstream ss ;
		ss << "Non è stato possibile caricarica il file " << nome_file;
	 	throw ( ss.str() ) ;

	}

    // la prima riga è l'header
    std::getline( ifs, header ) ;

  	while ( !ifs.eof () ) {

		std::string str;
		std::stringstream ss ;

		std::getline( ifs, str ) ;
		ss.precision(10);
		ss << str ;

        Data d ;
		ss.precision(10);
		ss >> d.i1 >> d.i2 >> d.i3 >> d.i4 >> d.i5 >> d.PHI >> d.A ;

        data.push_back( d );

	}

	ifs.close();

}

double ELP_GROUP2::sumSin ( double T )  const
{

    DelaunayLinear d ( T );

	double D     = d.D;
	double l1    = d.l1;
	double l     = d.l;
	double F     = d.F ;
	double Chi     = d.Chi ;

	double sum = 0;
	for ( size_t i = 0; i < data.size(); ++ i) {

        short i1 = data[i].i1;
        short i2 = data[i].i2;
        short i3 = data[i].i3;
        short i4 = data[i].i4;
        short i5 = data[i].i5;

        double phi = data[i].PHI;
        double A = data[i].A;

        Radiant arg = Grade ( i1 * Chi + i2 * D + i3 * l1 + i4 * l + i5 * F + phi ) ;

        sum += A * sin( arg );

	}

    return sum ;

}

double ELP_GROUP2::sumCos ( double T )  const
{

    DelaunayLinear d ( T );

	double D     = d.D;
	double l1    = d.l1;
	double l     = d.l;
	double F     = d.F ;
	double Chi     = d.Chi ;

	double sum = 0;
	for ( size_t i = 0; i < data.size(); ++ i) {

        short i1 = data[i].i1;
        short i2 = data[i].i2;
        short i3 = data[i].i3;
        short i4 = data[i].i4;
        short i5 = data[i].i5;

        double phi = data[i].PHI;
        double A = data[i].A;

        Radiant arg = Grade ( i1 * Chi + i2 * D + i3 * l1 + i4 * l + i5 * F + phi ) ;
        sum += A * cos( arg ) ;

	}

    return sum ;

}

// group 3

ELP_GROUP3::ELP_GROUP3 ( const std::string & nome_file ) :
    ELP( nome_file )
{

	std::ifstream ifs ( file_name.c_str(), std::ifstream::in );

  	if ( !ifs.good() ) {

		std::cout << " good()=" << ifs.good();
		std::cout << " eof()=" << ifs.eof();
		std::cout << " fail()=" << ifs.fail();
		std::cout << " bad()=" << ifs.bad();
		std::cout << " errore " << file_name.c_str() << std::endl;

		ifs.close();

		std::stringstream ss ;
		ss << "Non è stato possibile caricarica il file " << nome_file;
	 	throw ( ss.str() ) ;

	}

    // la prima riga è l'header
    std::getline( ifs, header ) ;

  	while ( !ifs.eof () ) {

		std::string str;
		std::stringstream ss ;

		std::getline( ifs, str ) ;
		ss.precision(10);
		ss << str ;

        Data d ;
		ss.precision(10);
		ss >> d.i1 >> d.i2 >> d.i3 >> d.i4 >> d.i5 >>
            d.i6 >> d.i7 >> d.i8 >> d.i9 >> d.i10 >> d.i11 >>
            d.PHI >> d.A ;

        data.push_back( d );

	}

	ifs.close();

}

double ELP_GROUP3::sumSin ( double TD )  const
{

    PlanetaryLongitudes p ( TD );

	double Me    = p.Me;
	double V     = p.V;
	double T     = p.T;
	double Ma    = p.Ma;
	double J     = p.J ;
	double S     = p.S ;
	double U     = p.U ;
	double N     = p.N ;

    DelaunayLinear d ( TD );

	double D     = d.D;
//	double l1    = d.l1;
	double l     = d.l;
	double F     = d.F ;
//	double Chi   = d.Chi ;

	double sum = 0;
	for ( size_t i = 0; i < data.size(); ++ i) {

        short i1 = data[i].i1;
        short i2 = data[i].i2;
        short i3 = data[i].i3;
        short i4 = data[i].i4;
        short i5 = data[i].i5;
        short i6 = data[i].i6;
        short i7 = data[i].i7;
        short i8 = data[i].i8;
        short i9 = data[i].i9;
        short i10 = data[i].i10;
        short i11 = data[i].i11;

        double phi = data[i].PHI;
        double A = data[i].A;

        Radiant arg = Grade ( i1 * Me + i2 * V + i3 * T + i4 * Ma + i5 * J +
                i6 * S + i7 * U + i8 * N + i9 * D + i10 * l + i11 * F + phi );
        sum += A * sin( arg );

	}

    return sum ;

}

double ELP_GROUP3::sumCos ( double TD )  const
{

    PlanetaryLongitudes p ( TD );

	double Me    = p.Me;
	double V     = p.V;
	double T     = p.T;
	double Ma    = p.Ma;
	double J     = p.J ;
	double S     = p.S ;
	double U     = p.U ;
	double N     = p.N ;

    DelaunayLinear d ( TD );

	double D     = d.D;
//	double l1    = d.l1;
	double l     = d.l;
	double F     = d.F ;
//	double Chi   = d.Chi ;

	double sum = 0;
	for ( size_t i = 0; i < data.size(); ++ i) {

        short i1 = data[i].i1;
        short i2 = data[i].i2;
        short i3 = data[i].i3;
        short i4 = data[i].i4;
        short i5 = data[i].i5;
        short i6 = data[i].i6;
        short i7 = data[i].i7;
        short i8 = data[i].i8;
        short i9 = data[i].i9;
        short i10 = data[i].i10;
        short i11 = data[i].i11;

        double phi = data[i].PHI;
        double A = data[i].A;

        Radiant arg = Grade ( i1 * Me + i2 * V + i3 * T + i4 * Ma + i5 * J +
                i6 * S + i7 * U + i8 * N + i9 * D + i10 * l + i11 * F + phi );
        sum += A * cos( arg );

	}

    return sum ;

}

// group 4

ELP_GROUP4::ELP_GROUP4 ( const std::string & nome_file ) :
    ELP_GROUP3 ( nome_file )
{
}

double ELP_GROUP4::sumSin ( double TD )  const
{

    PlanetaryLongitudes p ( TD );

	double Me    = p.Me;
	double V     = p.V;
	double T     = p.T;
	double Ma    = p.Ma;
	double J     = p.J ;
	double S     = p.S ;
	double U     = p.U ;
//	double N     = p.N ;

    DelaunayLinear d ( TD );

	double D     = d.D;
	double l1    = d.l1;
	double l     = d.l;
	double F     = d.F ;
//	double Chi   = d.Chi ;

	double sum = 0;
	for ( size_t i = 0; i < data.size(); ++ i) {

        short i1 = data[i].i1;
        short i2 = data[i].i2;
        short i3 = data[i].i3;
        short i4 = data[i].i4;
        short i5 = data[i].i5;
        short i6 = data[i].i6;
        short i7 = data[i].i7;
        short i8 = data[i].i8;
        short i9 = data[i].i9;
        short i10 = data[i].i10;
        short i11 = data[i].i11;

        double phi = data[i].PHI;
        double A = data[i].A;

        Radiant arg = Grade ( i1 * Me + i2 * V + i3 * T + i4 * Ma + i5 * J +
                i6 * S + i7 * U + i8 * D + i9 * l1 + i10 * l + i11 * F + phi );
        sum += A * sin( arg );

	}

    return sum ;

}

double ELP_GROUP4::sumCos ( double TD )  const
{

    PlanetaryLongitudes p ( TD );

	double Me    = p.Me;
	double V     = p.V;
	double T     = p.T;
	double Ma    = p.Ma;
	double J     = p.J ;
	double S     = p.S ;
	double U     = p.U ;
//	double N     = p.N ;

    DelaunayLinear d ( TD );

	double D     = d.D;
	double l1    = d.l1;
	double l     = d.l;
	double F     = d.F ;
//	double Chi   = d.Chi ;

	double sum = 0;
	for ( size_t i = 0; i < data.size(); ++ i) {

        short i1 = data[i].i1;
        short i2 = data[i].i2;
        short i3 = data[i].i3;
        short i4 = data[i].i4;
        short i5 = data[i].i5;
        short i6 = data[i].i6;
        short i7 = data[i].i7;
        short i8 = data[i].i8;
        short i9 = data[i].i9;
        short i10 = data[i].i10;
        short i11 = data[i].i11;

        double phi = data[i].PHI;
        double A = data[i].A;

        Radiant arg = Grade ( i1 * Me + i2 * V + i3 * T + i4 * Ma + i5 * J +
                i6 * S + i7 * U + i8 * D + i9 * l1 + i10 * l + i11 * F + phi );
        sum += A * cos( arg );

	}

    return sum ;

}

// group 5


ELP_GROUP5::ELP_GROUP5 ( const std::string & nome_file ) :
    ELP_GROUP2( nome_file )
{
}

const double Delaunay::p = 5028.792   ;  // general precession jd2000 mas x century
Delaunay::Delaunay ( double TD ) :
    TDB ( TD )
{
}

void Delaunay::Init ()
{

    D   = W1() - T() + 180 ;
    l1  = T( ) - Omega1();
    l   = W1() - W2() ;
    F   = W1() - W3();
    Chi = W1() + p * TDB;

}

DelaunayComplete::DelaunayComplete ( double TD ) :
    Delaunay ( TD )
{
    Init();
}

double DelaunayComplete::W1 ( ) {

		Grade a0 ( 218, 18, 59.95571) ;

		double a1 = 1732559343.73604 ;
		double a2 = -5.8883 ;
		double a3 = 0.006604 ;
		double a4 = -0.00003169 ;

		double sec = a1 * TDB + a2 * pow( TDB, 2 ) + a3 * pow ( TDB, 3 ) + a4 * pow( TDB, 4 );
		Grade gradi = a0 + sec / 3600 ;

		return gradi;

}

double DelaunayComplete::W2 ( )  {

		Grade a0 ( 83, 21, 11.67475) ;

		double a1 = 14643420.2632 ;
		double a2 = -38.2776 ;
		double a3 = 0.045047 ;
		double a4 = 0.00021301 ;

		double sec = a1 * TDB + a2 * pow( TDB, 2 ) + a3 * pow ( TDB, 3 ) + a4 * pow( TDB, 4 );
		Grade gradi = a0 + sec / 3600 ;

		return gradi;

}


double DelaunayComplete::W3 ( ) {

		Grade a0 ( 125, 02, 40.39816) ;

		double a1 = -6967919.3622 ;
		double a2 = 6.3622 ;
		double a3 = 0.007625 ;
		double a4 = -0.00003586 ;

		double sec = a1 * TDB + a2 * pow( TDB, 2 ) + a3 * pow ( TDB, 3 ) + a4 * pow( TDB, 4 );
		Grade gradi = a0 + sec / 3600 ;

		return gradi;

}


double DelaunayComplete::T ( )  {

		Grade a0 ( 100, 27, 59.22059) ;

		double a1 = 129597742.2758 ;
		double a2 = -0.0202 ;
		double a3 = 0.000009 ;
		double a4 = 0.00000015 ;

		double sec = a1 * TDB + a2 * pow( TDB, 2 ) + a3 * pow ( TDB, 3 ) + a4 * pow( TDB, 4 );
		Grade gradi = a0 + sec / 3600 ;

		return gradi;

}


double DelaunayComplete::Omega1 ( )  {

		Grade a0 ( 102, 56, 14.42753) ;

		double a1 = 1161.2283 ;
		double a2 = 0.5327 ;
		double a3 = -0.000138 ;

		double sec = a1 * TDB + a2 * pow( TDB, 2 ) + a3 * pow ( TDB, 3 ) ;
		Grade gradi = a0 + sec / 3600 ;

		return gradi;

}

double DelaunayComplete::pA ( )
{

		double a1 = 5029.0966 ;
		double a2 = 1.1120 ;
		double a3 = 0.000077 ;
		double a4 = -0.00002353 ;

		double sec = a1 * TDB + a2 * pow( TDB, 2 ) + a3 * pow ( TDB, 3 ) + a4 * pow( TDB, 4 );
		Grade gradi = sec / 3600 ;

		return gradi;

}

DelaunayLinear::DelaunayLinear ( double TD ) :
    Delaunay ( TD )
{
    Init();
}

double DelaunayLinear::W1 ( ) {

		Grade a0 ( 218, 18, 59.95571) ;

		double a1 = 1732559343.73604 ;

		double sec = a1 * TDB ;
		Grade gradi = a0 + sec / 3600 ;

		return gradi;

}

double DelaunayLinear::W2 ( )  {

		Grade a0 ( 83, 21, 11.67475) ;

		double a1 = 14643420.2632 ;

		double sec = a1 * TDB;
		Grade gradi = a0 + sec / 3600 ;

		return gradi;

}


double DelaunayLinear::W3 ( ) {

		Grade a0 ( 125, 02, 40.39816) ;

		double a1 = -6967919.3622 ;

		double sec =  a1 * TDB ;
		Grade gradi = a0 + sec / 3600 ;

		return gradi;

}


double DelaunayLinear::T ( )  {

		Grade a0 ( 100, 27, 59.22059) ;

		double a1 = 129597742.275 ;

		double sec = a1 * TDB;
		Grade gradi = a0 + sec / 3600 ;

		return gradi;

}


double DelaunayLinear::Omega1 ( )  {

		Grade a0 ( 102, 56, 14.42753) ;

		double a1 = 1161.2283 ;

		double sec = a1 * TDB  ;
		Grade gradi = a0 + sec / 3600 ;

		return gradi;

}

PlanetaryLongitudes::PlanetaryLongitudes ( double TD )
{

		Grade me ( 252, 15, 03.25986 ) ;
		double l1 = 538101628.68898 / 3600 ;

		Me = me + l1 * TD;

		Grade v ( 181, 58, 47.28305 ) ;
		l1 = 210664136.43355 / 3600 ;

        V = v + l1 * TD;

		Grade t ( 100, 27, 59.22059 ) ;
		l1 = 129597742.27580 / 3600 ;

        T = t + l1 * TD;

		Grade ma ( 355, 25, 59.78866 ) ;
		l1 = 68905077.59284 / 3600 ;

        Ma = ma + l1 * TD;

		Grade j ( 34, 21, 05.34212) ;
		l1 = 10925660.42861 / 3600 ;

        J = j + l1 * TD;

		Grade s ( 50, 04, 38.89694) ;
		l1 = 4399609.65932 / 3600 ;

        S = s + l1 * TD;

		Grade u ( 314, 03, 18.01841 ) ;
		l1 = 1542481.19393 / 3600 ;

        U = u + l1 * TD;

		Grade n ( 304, 20, 55.19575 ) ;
        l1 = 786550.32074 / 3600 ;

        N = n + l1 * TD;


}







