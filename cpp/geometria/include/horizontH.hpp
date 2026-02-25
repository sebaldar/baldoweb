#ifndef _horizont_
#define _horizont_

#include <point.hpp>
#include <line.hpp>
#include <iostream>
#include <math.h>

class Horizont  {


	const double DELTA = 10;

	double
		latitude,
		longitude ;
		

	Point 
		p_north ,
		p_east,
		p_south,
		p_west ;

	// linea direttive
	StraightLine 
			line_south_north,
			line_west_east,
			line_north_south,
			line_east_west ;
			
	Point pdv ;	// punto di vista
	
	// coordinate punto di osservazione
	double x_pdv, y_pdv, z_pdv;
	
	double r_cono ;
	double h_cono ;


	// funzione che calcola corregge il punto di vista a seconda della distanza imposta dalla vista sull'orizzonte
	Point height_correction ( const Point & ph, double height) const ; 
	
public:

	// double H; altezza sul livello del mare in metri
	Horizont ( double latitude, double longitude, double H ) ;
	
	Point lookatAzimut ( double azimut, double altezza = 0 ) const ;
	
	
	double azimut ( const Point & ) ;
	double height ( const Point & ) ;
	
	std::string track ( bool show ) ;	// traccia la primitiva della linea d'orizzonte
	Point pointOfView () {
			return pdv;
		}
	Plane horizont;	// piano dell'orizzonte a latitude e longitude
		
};


#endif	// if _horizont_
