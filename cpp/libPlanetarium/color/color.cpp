#include <sstream>
#include <math.h>

#include <color.hpp>

RGB::RGB( float r, float g, float b )  : 
	R (r), G( g), B( b ) {
}
	
RGB::RGB( const class RGB & rgb ) :
	RGB( rgb.R, rgb.G, rgb.B )
{
}

RGB::RGB( BV & _bv ) 
{

	float bv = _bv;
	// RGB <0,1> <- BV <-0.4,+2.0> [-]
		
	float t;
	float r = 0.0, g = 0.0, b = 0.0;
		
	if (bv<-0.4) bv=-0.4;
	if (bv> 2.0) bv= 2.0;
		
	if ((bv>=-0.40)&&(bv<0.00)) { 
		t = (bv+0.40)/(0.00+0.40);
		r = 0.61+(0.11*t)+(0.1*t*t);
	}
	else if ((bv>= 0.00)&&(bv<0.40)) {
		t=(bv-0.00)/(0.40-0.00);
		r=0.83+(0.17*t);
	}
	else if ( (bv>= 0.40)&&(bv<2.10) ) {
		t=(bv-0.40)/(2.10-0.40);
		r=1.00 ;
	}

	if ( (bv>=-0.40)&&(bv<0.00) ) {
			
		t=(bv+0.40)/(0.00+0.40);
		g=0.70+(0.07*t)+(0.1*t*t);

	}
	else if ( (bv>= 0.00)&&(bv<0.40) ) {
		t=(bv-0.00)/(0.40-0.00);
		g=0.87+(0.11*t) ;
	}
	else if ( (bv>= 0.40)&&(bv<1.60) ) {
		t=(bv-0.40)/(1.60-0.40);
		g=0.98-(0.16*t)  ;
	}
	else if ( (bv>= 1.60)&&(bv<2.00) ) {
		t=(bv-1.60)/(2.00-1.60);
		g=0.82 -(0.5*t*t);
	}

	if ( (bv>=-0.40)&&(bv<0.40) ) {
		t=(bv+0.40)/(0.40+0.40);
		b=1.00  ;
	}
	else if ((bv>= 0.40)&&(bv<1.50)) {
		t=(bv-0.40)/(1.50-0.40);
		b=1.00-(0.47*t)+(0.1*t*t);
	}
	else if ((bv>= 1.50)&&(bv<1.94)) {
		t=(bv-1.50)/(1.94-1.50);
		b=0.63 -(0.6*t*t);
	}
		
	R = r, G = g, B = b ;

}

RGB::RGB( const class HSV & hsv ) 
{
	*this = hsv;
}

RGB::operator std::string () 
{
			
	std::string r, g, b;

	int 
		red = R * 255,
		green = G * 255,
		blue = B * 255;
		
	std::stringstream ss;
	ss << std::hex << red << " " << green << " " << blue ;
	ss >>  r >> g >> b;
		
	r = "0" + r;
	g = "0" + g;
	b = "0" + b;

	std::stringstream t ;
	t << "0x" << r.substr( r.size()-2, 2) << g.substr( g.size()-2, 2) << b.substr( b.size()-2, 2)   ;
	return t.str();

}


RGB &RGB::operator = ( const RGB &rgb)
{

	R = rgb.R;
	G = rgb.G;
	B = rgb.B;
	
	return *this;
	
}

RGB &RGB::operator = ( const HSV &hsv)
{

	float 
		fH = hsv.H,
		fS = hsv.S,
		fV = hsv.V;
		
	float fC = fV * fS; // Chroma
	float fHPrime = fmod(fH / 60.0, 6);
	float fX = fC * (1 - fabs(fmod(fHPrime, 2) - 1));
	float fM = fV - fC;

	if(0 <= fHPrime && fHPrime < 1) {
		R = fC;
		G = fX;
		B = 0;
	} else if(1 <= fHPrime && fHPrime < 2) {
		R = fX;
		G = fC;
		B = 0;
	} else if(2 <= fHPrime && fHPrime < 3) {
		R = 0;
		G = fC;
		B = fX;
	} else if(3 <= fHPrime && fHPrime < 4) {
		R = 0;
		G = fX;
		B = fC;
	} else if(4 <= fHPrime && fHPrime < 5) {
		R = fX;
		G = 0;
		B = fC;
	} else if(5 <= fHPrime && fHPrime < 6) {
		R = fC;
		G = 0;
		B = fX;
	} else {
		R = 0;
		G = 0;
		B = 0;
	}

	R += fM;
	G += fM;
	B += fM;
	
	return *this;
	
}


HSV::HSV( float h, float s, float v )  : 
	H (h), S( s), V( v ) {
}
	
HSV::HSV( const class RGB & rgb ) 
{
	*this = rgb;
}

HSV::operator std::string () 
{
			
	std::stringstream t ;
	return t.str();

}

HSV &HSV::operator = ( const RGB &rgb)
{

	float 
		fR = rgb.R,
		fG = rgb.G,
		fB = rgb.B;
		
	float fCMax = std::max( std::max( fR, fG), fB );
	float fCMin = std::min(std::min(fR, fG), fB);

	float fDelta = fCMax - fCMin;
  
	if(fDelta > 0) {
		if(fCMax == fR) {
			H = 60 * (fmod(((fG - fB) / fDelta), 6));
		} else if(fCMax == fG) {
			H = 60 * (((fB - fR) / fDelta) + 2);	
		} else if(fCMax == fB) {
			H = 60 * (((fR - fG) / fDelta) + 4);
		}

		if( fCMax > 0 ) {
			S = fDelta / fCMax;
		}	else {
			S = 0;
		}

		V = fCMax;

	}	else {
   
		H = 0;
		S = 0;
		V = fCMax;
	}
  
	if( H < 0 ) {
		H = 360 + H;
	}

	return *this;

}

