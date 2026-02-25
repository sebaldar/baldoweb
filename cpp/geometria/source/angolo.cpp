#include <sstream>
#include <angolo.hpp>

Angle::Angle ( ) :
	Angle ( 0 )
{
}

Angle::Angle ( double val_radiant )
{

	radiant = normalize ( val_radiant );

}

Angle::~Angle ()
{
}

void Angle::setGrade ( const double & val_grade )
{

	radiant = normalize ( val_grade * pi / 180 );

}

double Angle::normalize ( double radiant )
{

    const double two_pi = 2 * pi;
    return radiant - two_pi * floor( ( radiant + pi ) / two_pi );


}

double Angle::toGrade ()
{

    return radiant * 180 / ( pi ) ;

}

std::string Angle::toGradeHMS ()
{
    double val = toGrade ();
    double in_24_ore = val * 24 / 360;
    
    double h = floor(in_24_ore) ;
    double v = in_24_ore - h;
    double m = floor( v * 60 );
    double s = (v * 60 - m) * 60;
    s = floor ( ( s + 0.5) * 100 ) / 100; 

	if ( h < 0 ) h = h + 24;
	std::stringstream result;
	result << h << "h" << m << "m" << s << "s" ;
	return result.str();


}

void Angle::setGradeGPS ( double gradi, double primi, double secondi )
{

    Grade g = ( gradi + primi / 60 + secondi / 3600 ) ;
    radiant = Radiant( g );

}

std::string Angle::toGradeGPS ()
{

    double sign = radiant / fabs( radiant );
    // in gradi
    double gradi = fabs( radiant ) * 180 / pi;
    if ( gradi > 180 ) gradi = 360 - gradi;
    double g = floor ( gradi ) ;
    double p = floor ( (   gradi - g ) * 60 ) ;
    double s = ( ( gradi - g ) * 60 - p  ) * 60 ;
    s = floor ( ( s + 0.5) * 100 ) / 100; 

	std::stringstream result;
    result.precision(10);
	result << sign * g << "°" << p << "'" << s << "''" ;
	return result.str();


}

double Angle::normalize (  )
{
    return radiant = normalize( radiant );
}
/*
Angle Angle::operator + ( const Angle & a1 )
{
	return Angle ( radiant + a1.radiant );
}

Angle Angle::operator - ( const Angle & a1 )
{
	return Angle ( radiant - a1.radiant );
}
*/


Angle &Angle::operator = ( const Angle &v)
{
	radiant = v.radiant;
	return *this;
}

Angle &Angle::operator = ( double r)
{
    radiant = normalize( r );
	return *this;
}

Radiant::Radiant ( double val_radiant ) :
    Angle ( val_radiant )
{
}

Radiant::Radiant ( const Grade & g ) :
    Angle ( )
{
    radiant = g.radiant;
}

Radiant::Radiant ( const Angle & a ) :
    Angle ( )
{
    radiant = a.radiant;
}

Radiant &Radiant::operator = ( const double &v)
{

    radiant = normalize( v );
	return *this;

}

Radiant &Radiant::operator = ( const Grade &v) {
    radiant = v.radiant;
	return *this;
}

Radiant &Radiant::operator = ( const Angle &a) {
    radiant = a.radiant;
	return *this;
}

Grade::Grade ( const double & grade ) :
    Angle ( )
{

    setGrade ( grade );

}

Grade::Grade ( double gradi, double primi, double secondi )  :
    Angle ( )
{
	setGradeGPS ( gradi, primi, secondi ) ;
}

Grade::Grade ( const Radiant & r ) :
    Angle ( )
{
    radiant = r.radiant;
}

Grade::Grade ( const Angle & a ) :
    Angle ( )
{
    radiant = a.radiant;
}

Grade::Grade ( const Grade & g ) :
    Angle ( )
{
    radiant = g.radiant;
}

Grade &Grade::operator = ( const Radiant &v) {
    radiant = v.radiant;
    return *this;
}

Grade &Grade::operator = ( const Angle &a) {
    radiant = a.radiant;
    return *this;
}

Grade &Grade::operator = ( const Grade &a) {
    radiant = a.radiant;
    return *this;
}

Grade &Grade::operator = ( const double & grade)
{

    setGrade ( grade );
	return *this;

}





