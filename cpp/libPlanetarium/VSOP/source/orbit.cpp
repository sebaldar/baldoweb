#include <math.h>       /* asin */

#include <celestial_body.hpp>
#include <orbit.hpp>

OrbitXYZ::OrbitXYZ () 
{
}

OrbitXYZ::OrbitXYZ ( double _x, double _y, double _z ) :
	x ( _x ), y( _y ), z ( _z )
{
}

OrbitXYZ::OrbitXYZ ( const struct OrbitEqu & orbit) 
{
	*this = orbit;
}

OrbitXYZ::OrbitXYZ ( const struct OrbitEcli & orbit) 
{
	*this = orbit;
}

OrbitXYZ &OrbitXYZ::operator = ( const OrbitEqu & orbit )
{
		
	Grade delta = orbit.delta;
	Grade alfa = orbit.alfa;
	double R = orbit.R ;
	
	x = R * cos( Radiant(delta) ) * cos( Radiant(alfa ) ) ;
	y = R * cos( Radiant(delta) ) * sin( Radiant(alfa ) );
	z = R * sin( Radiant(delta) );
    
	return *this;

}

OrbitXYZ &OrbitXYZ::operator = ( const OrbitEcli & orbit )
{
		
	Grade lambda = orbit.L;
	Grade beta = orbit.B;
	double R = orbit.R ;
	
	x = R * cos( Radiant(lambda) ) * cos( Radiant(beta ) ) ;
	y = R * cos( Radiant(lambda) ) * sin( Radiant(beta ) );
	z = R * sin( Radiant(lambda) );
    
	return *this;
}

OrbitXYZ &OrbitXYZ::operator = ( const OrbitXYZ & orbit )
{
		
	x = orbit.x ;
	y = orbit.y;
	z = orbit.z;
    
	return *this;
}

double OrbitXYZ::R()
{
	return pow( x * x + y * y + z * z, 0.5 );
}

OrbitXYZ OrbitXYZ::toEclittic ( double JD )  const
{
	Earth earth;
	Grade epsilon = earth.obliquity ( JD );
	Radiant eps = epsilon ;
	
	OrbitXYZ ecli;
	ecli.x = x;
	ecli.y = y * cos( eps ) + z * sin( eps );
	ecli.z = z * cos( eps ) - y * sin( eps );
	
	ecli.vx = vx;
	ecli.vy = vy * cos( eps ) + vz * sin( eps );
	ecli.vz = vz * sin( eps ) + vy * cos( eps );
	
	return ecli;
	
}

OrbitXYZ OrbitXYZ::toEqutorial ( double JD ) const
{
	Earth earth;
	Grade epsilon = earth.obliquity ( JD );
	Radiant eps = epsilon ;
	
	OrbitXYZ equ;
	equ.x = x;
	equ.y = y * cos( eps ) - z * sin( eps );
	equ.z = y * sin( eps ) + z * cos( eps );
	
	equ.vx = vx;
	equ.vy = vy * cos( eps ) - vz * sin( eps );
	equ.vz = vy * sin( eps ) + vz * cos( eps );
	
	return equ;
	
}

OrbitEqu::OrbitEqu ( double a, double d, double r ) :
	alfa ( a ), delta ( d ), R ( r )
{
}
	
OrbitEqu::OrbitEqu ( const OrbitXYZ & orbit )   :
	alfa ( 0 ), delta ( 0 ), R ( 0 )
{
	*this = orbit ;
}
	
OrbitEqu::OrbitEqu ()  :
	alfa ( 0 ), delta ( 0 ), R ( 0 )
{
}

OrbitEqu::OrbitEqu ( double jd )  :
	alfa ( 0 ), delta ( 0 ), R ( 0 )
{

	Earth earth;
/*
	Earth::Nutation n = earth.nutation( jd );
	epsilon = earth.obliquity( jd ) - n.Depsilon;
*/
	epsilon = earth.obliquity( jd ) ;

}
/*
OrbitEqu &OrbitEqu::operator = ( const OrbitAlfa & orbit )
{

	Radiant lambda = Grade( orbit.lambda );
	Radiant beta = Grade( orbit.beta );
	Radiant e = Grade( epsilon );
	
	double tan_alfa = ( sin( lambda ) * cos( e ) - tan( beta ) * sin( e ) ) / cos( lambda );
	double sin_delta = sin( beta ) * cos( e ) + cos( beta ) * sin( e ) * sin( lambda );
	
	Radiant a = atan( tan_alfa ) ;
	Radiant d = asin( sin_delta );
	
	R = orbit.delta;
	alfa = Grade ( a );
	double g_lambda = Grade(lambda);

	// se i valori sono lontani correggili
	if ( fabs( g_lambda - alfa ) > 10 ) {
		if ( g_lambda > 0 ) alfa = alfa + 180;
		else alfa = alfa - 180;
	}
	
	delta = Grade ( d ); 
	
	return *this;
	
	
}
*/
OrbitEqu &OrbitEqu::operator = ( const OrbitEcli & orbit )
{

	Radiant lambda = Grade( orbit.L );
	Radiant beta = Grade( orbit.B );
	Radiant e = Grade( epsilon );
	
	double tan_alfa = ( sin( lambda ) * cos( e ) - tan( beta ) * sin( e ) ) / cos( lambda );
	double sin_delta = sin( beta ) * cos( e ) + cos( beta ) * sin( e ) * sin( lambda );
	
	
	Radiant a = atan( tan_alfa ) ;
	Radiant d = asin( sin_delta );
	
	R = orbit.R;
	alfa = Grade ( a ) ;
	double g_lambda = Grade(lambda);

	// se i valori sono lontani correggili
	if ( fabs( g_lambda - alfa ) > 10 ) {
		if ( g_lambda > 0 ) alfa = alfa + 180;
		else alfa = alfa - 180;
	}
	
	delta = Grade ( d ); 
	
	return *this;
	
	
}

OrbitEqu &OrbitEqu::operator = ( const OrbitXYZ & orbit )
{

	double x = orbit.x ;
	double y = orbit.y ;
	double z = orbit.z ;
				
	double r_xy = pow( pow( x, 2 ) + pow( y, 2 ), 0.5 );
	Declinazione Dec = Radiant( atan( z / r_xy ) );
	
	// determina il quadrante
	bool sign_y = fabs(y) / y > 0 ? true : false;
	bool sign_x = fabs(x) / x > 0 ? true : false;
	short quadrante = 0;
	
	if ( sign_x && sign_y  ) quadrante = 1;
	else if ( !sign_x && sign_y ) quadrante = 2;
	else if ( !sign_x && !sign_y ) quadrante = 3;
	else if ( sign_x && !sign_y ) quadrante = 4;
				
	Grade AR  = Radiant( atan( fabs(y) / fabs(x) ) );
	
	switch ( quadrante ) {
		case 2 : AR = 180 - AR ;
		break;
		case 3 : AR = 180 + AR ;
		break;
		case 4 : AR = 360 - AR ;
		break;
		default :
		break;
	}
		
	delta = Dec ;
	alfa = AR ;	
	R = pow ( x * x + y * y + z * z, 0.5 ) ;	
	
	return *this;
	
	
}
