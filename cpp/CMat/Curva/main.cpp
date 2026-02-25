#include <iostream>
#include <sstream>

#include <stdlib.h>
#include <string.h>

#include "gd.h"
#include "gdfontt.h"

using namespace std;

#include <cgi_base.hpp>
#include <Curva.h>

#ifndef __xml__
	#include "xml.hpp"
#endif

int main (int argc, char *argv[])
{


	void plot ( Curva * curva ) ;

	CGI * cgi = new CGI;
	string documento = cgi->query_string();

	XMLDocument doc ;
	doc.parse( documento );

	XMLElement *root = doc.rootElement();

	string alg = root->getAttribute( "algoritmo");
	string nome = root->getAttribute( "nome");
	string descrizione = root->getAttribute( "descrizione");

	Curva * curva ;

	if ( alg == "lineare" )
		curva = new LCurva ( root );
	else if ( alg == "poly" )
		curva = new PCurva ( root );
	else if ( alg == "cspline" )
		curva = new CSICurva ( root );
	else
		curva = new CSICurva ( root );

	plot ( curva );
	delete curva;

	return ( 0 );


}

void plot ( Curva * curva ) {

	const size_t shift_y = 150;
	const size_t shift_x = 150;
	
	size_t r=255, g=255, b=255;
	size_t w=500, h=500;
	Limiti lim = curva->limiti();

	if ( lim.min_x > 0 ) lim.min_x = 0;
	if ( lim.min_y > 0 ) lim.min_y = 0;

	REAL dx = ( lim.max_x - lim.min_x ) ;
	REAL dy = ( lim.max_y - lim.min_y ) ;

	REAL x0 = lim.min_x * w / dx;
	REAL y0 = lim.min_y * h / dy;

	size_t h1 = h + shift_y;
	gdImagePtr im = gdImageCreate( w+100, h1 );
	/* Background color (first allocated) */
	int background_color = gdImageColorResolve( im, r, g, b );
	int black = gdImageColorResolve( im, 0, 0, 0 );
	int red = gdImageColorResolve( im, 255, 0, 0 );

	gdImageSetThickness(im, 2);
	gdImageLine( im, shift_x, h1 - shift_y, w + shift_x,  h1 - shift_y, black );	// ascissa
	gdImageLine( im, shift_x, h1 - shift_y, shift_x, shift_y, black );	// ordinata

	for ( size_t i=0; i <= w+shift_x; i++ ) {

		REAL x = i * dx / w ;
		REAL y = curva->value(x) ;

		gdImageSetPixel( im, shift_x + x * w / dx, h1 - y * h / dy - shift_y, black ) ;

	}

	// inserisce il punti reali
	for ( size_t i = 0; i < curva->getEl(); i++) {

		REAL x = curva->getPoint( i ).x ;
		REAL y = curva->getPoint( i ).y ;

		size_t cx = shift_x + x * w / dx;
		size_t cy = h1 - y * h / dy - shift_y;

		gdImageFilledArc(im, cx, cy, 3, 3, 0, 360, red, gdArc);

		ostringstream os1 ( "" );
		os1 << x;
		ostringstream os2 ( "" );
		gdImageLine( im, cx, h1 - shift_y , cx, h1 - shift_y - 3, black );
		gdImageString( im, gdFontGetTiny(), cx, h1 - shift_y + 2, (unsigned char *) os1.str().c_str(), black);
		os2 << y;
		gdImageLine( im, shift_x, cy, shift_x + 3, cy, black );
		gdImageString( im, gdFontGetTiny(), shift_x + 4, cy, (unsigned char *) os2.str().c_str(), black);

	}


	gdImagePng(im, stdout);

	/* Destroy it */
	gdImageDestroy(im);

}
