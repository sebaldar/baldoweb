#include <fstream>
#include <sstream>
#include <math.h>       /* cos */

#include <date.hpp>
#include <vsop.hpp>

VSOP::VSOP ( const std::string & file_name )
{

	std::ifstream ifs ( file_name.c_str(), std::ifstream::in );

  	if ( !ifs.good() ) {

		std::stringstream ss ;
		ss << "Errore di lettura del file " << file_name << std::endl;
		ss << " good()=" << ifs.good();
		ss << " eof()=" << ifs.eof();
		ss << " fail()=" << ifs.fail();
		ss << " bad()=" << ifs.bad();
		ss << std::endl;

		ifs.close();

		std::cerr << ss.str() << std::endl;

		throw ( ss.str() ) ;

	}

  	while ( !ifs.eof () ) {

		std::string str;
		std::stringstream ss ;

		std::getline( ifs, str ) ;
		ss << str ;

		std::string header_tag ;
		ss >> header_tag;
		if ( header_tag == "VSOP87" ) {

			// è un header
			std::string version, c3, name, variable, xyz, T;
			unsigned short tipo, size, alfa;
			ss >> version >> c3 >> name >> variable >> tipo >> xyz >> T >> size;

			alfa = atoi( T.substr(4, 1).c_str() );	// T è nella forma *T**

			Var * var;
			switch ( tipo ) {
				case 1 :	// var X
					var = & X[alfa];
				break;
				case 2 :	// var Y
					var = & Y[alfa];
				break;
				case 3 :	// var Z
					var = & Z[alfa];
				break;
			}

			var->n = size;

			// legge i dati
			for ( size_t i = 0; i < size; ++i ) {

				std::getline( ifs, str ) ;
				std::stringstream ss ;
				ss << str ;


				{

					// scarto i primi 16, mi servono solo gli ultimi 3
					double cod, n;
					ss >> cod >> n  ;

					unsigned short disc = 15;

					for ( size_t j = 0; j < disc; ++j ) {

						std::string str ;
						ss >> str;
						// se la lunghezza è > 2
						// ci sono due dati compattati
						// ne conteggio uno in meno
						if ( str.size() > 2 ) j++;

					}

				}

				double a, b, c;
				ss >> a >> b >> c;

				var->A.push_back( a );
				var->B.push_back( b );
				var->C.push_back( c );

			}

		}


	}

	ifs.close();

}

OrbitXYZ VSOP::getOrbit ( double JD ) const
{

//	double T = ( JD - 2451545.0 ) / 365250.0 ;
	double T = ( JD - JCentury::JD2000 ) / JCentury::JULIANMILLENIUN ;

	OrbitXYZ orbit;
	// coordinata X
	// per ogni alfa
	for ( size_t alfa = 0; alfa < 6; ++alfa ) {

		const Var & var = X[alfa];

		double p = 0;
		double v1 = 0, v2 = 0;

		unsigned short n = var.n;
		for ( size_t j = 0; j < n; ++j ) {

			double A = var.A[j];
			double B = var.B[j];
			double C = var.C[j];

			double c1 = A * cos( B + C * T );
			double c2 = A * C * sin( B + C * T );

			// posizione
			p  += c1 ;

			// velocità
			v1 += c1 ;
			v2 += c2 ;

		}

		orbit.x  += pow( T, alfa ) * p ;
		orbit.vx += alfa * pow( T, alfa - 1 ) * v1 - pow( T, alfa ) * v2 ;

	}

	for ( size_t alfa = 0; alfa < 6; ++alfa ) {

		const Var & var = Y[alfa];

		double p = 0;
		double v1 = 0, v2 = 0;

		unsigned short n = var.n;
		for ( size_t j = 0; j < n; ++j ) {

			double A = var.A[j];
			double B = var.B[j];
			double C = var.C[j];

			double c1 = A * cos( B + C * T );
			double c2 = A * C * sin( B + C * T );

			// posizione
			p  += c1 ;
			// velocità
			v1 += c1 ;
			v2 += c2 ;

		}

		orbit.y += pow( T, alfa ) * p ;
		orbit.vy += alfa * pow( T, alfa - 1 ) * v1 - pow( T, alfa ) * v2;


	}

	for ( size_t alfa = 0; alfa < 6; ++alfa ) {

		const Var & var = Z[alfa];

		double p = 0;
		double v1 = 0, v2 = 0;

		unsigned short n = var.n;
		for ( size_t j = 0; j < n; ++j ) {

			double A = var.A[j];
			double B = var.B[j];
			double C = var.C[j];

			double c1 = A * cos( B + C * T );
			double c2 = A * C * sin( B + C * T );

			// posizione
			p  += c1 ;
			// velocità
			v1 += c1 ;
			v2 += c2 ;

		}

		orbit.z += pow( T, alfa ) * p ;
		orbit.vz += alfa * pow( T, alfa - 1 ) * v1 - pow( T, alfa ) * v2;

	}

	orbit.vx /= JCentury::JULIANMILLENIUN;
	orbit.vy /= JCentury::JULIANMILLENIUN;
	orbit.vz /= JCentury::JULIANMILLENIUN;

	return orbit;

}

OrbitEcli VSOP::getOrbitLBR ( double JD ) const
{

//	double T = ( JD - 2451545.0 ) / 365250.0 ;
	double T = ( JD - JCentury::JD2000 ) / JCentury::JULIANMILLENIUN ;

	OrbitEcli orbit;
	// per ogni alfa
	for ( size_t alfa = 0; alfa < 6; ++alfa ) {

		const Var & var = X[alfa];

		double p = 0;
		double v1 = 0, v2 = 0;

		unsigned short n = var.n;
		for ( size_t j = 0; j < n; ++j ) {

			double A = var.A[j];
			double B = var.B[j];
			double C = var.C[j];

			double c1 = A * cos( B + C * T );
			double c2 = A * C * sin( B + C * T );

			// posizione
			p  += c1 ;

			// velocità
			v1 += c1 ;
			v2 += c2 ;

		}

		orbit.L  += pow( T, alfa ) * p ;

	}

	for ( size_t alfa = 0; alfa < 6; ++alfa ) {

		const Var & var = Y[alfa];

		double p = 0;
		double v1 = 0, v2 = 0;

		unsigned short n = var.n;
		for ( size_t j = 0; j < n; ++j ) {

			double A = var.A[j];
			double B = var.B[j];
			double C = var.C[j];

			double c1 = A * cos( B + C * T );
			double c2 = A * C * sin( B + C * T );

			// posizione
			p  += c1 ;
			// velocità
			v1 += c1 ;
			v2 += c2 ;

		}

		orbit.B += pow( T, alfa ) * p ;


	}

	for ( size_t alfa = 0; alfa < 6; ++alfa ) {

		const Var & var = Z[alfa];

		double p = 0;
		double v1 = 0, v2 = 0;

		unsigned short n = var.n;
		for ( size_t j = 0; j < n; ++j ) {

			double A = var.A[j];
			double B = var.B[j];
			double C = var.C[j];

			double c1 = A * cos( B + C * T );
			double c2 = A * C * sin( B + C * T );

			// posizione
			p  += c1 ;
			// velocità
			v1 += c1 ;
			v2 += c2 ;

		}

		orbit.R += pow( T, alfa ) * p ;

	}

	return orbit;

}

