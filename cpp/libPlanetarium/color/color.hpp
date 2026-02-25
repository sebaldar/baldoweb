#ifndef _color_
#define _color_

#include <iostream>

// indice di colore
// B-V: differenza tra la magnitudine della banda B e V
class BV {

	float bv;
	
	public:
	
	BV ( float _bv ) :
		bv ( _bv ) {
			}
	
	BV& operator = ( float t ) {
		bv = t;
		return *this;
	}
	operator float () {
		return bv;
	}

};

class RGB {
		
		friend class HSV;
	
	public :
		// RGB values are in the range [0, 1]
		float R, G, B;
		
	public :
		// ingresso 0-1
		RGB( float red, float green, float blue ) ;
		RGB( BV & ) ;
		RGB( const class RGB & ) ;
		RGB( const class HSV & ) ;
	
		operator std::string () ;
		RGB &operator = ( const class HSV & );
		RGB &operator = ( const class RGB & );
	
};


class HSV {
		
		friend class RGB;
	
	public :
	// HSV values are in the ranges h = [0, 360], and s, v = [0,1]
		float H, S, V;
		
	public :

		HSV( float red, float green, float blue ) ;
		HSV( const RGB & ) ;
	
		operator std::string () ;
		HSV &operator = ( const RGB &v);

};


#endif	// if _color_
