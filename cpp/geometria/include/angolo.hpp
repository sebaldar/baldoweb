#ifndef _angle_
#define _angle_

#include <iostream>
#include <math.h>

class Angle  {

protected:

public:

	static constexpr double pi = atan( 1 ) * 4 ;
	double radiant ;

	Angle ( ) ;
	Angle ( double val_radiant ) ;

	void setGrade ( const double & val_grade ) ;

	virtual ~Angle () ;

	double normalize ( double radiant );
	double normalize (  );

	double toGrade () ;
	std::string toGradeHMS () ;
	std::string toGradeGPS ( ) ;   // set in gradi primi secondi
	void setGradeGPS ( double gradi, double primi, double secondi ) ;   // set in gradi primi secondi

	Angle &operator = ( const Angle &v);
	Angle &operator = ( double v);

//	Angle operator + ( const Angle & a1 );
//	Angle operator - ( const Angle & a1 );

    operator double () { return radiant; }

 };

class Radiant : public Angle {

public:

 	Radiant ( double val_radiant ) ;
 	Radiant ( const class Grade & g );
 	Radiant ( const class Angle & a );

	Radiant &operator = ( const double &v);
	Radiant &operator = ( const class Grade &v) ;
	Radiant &operator = ( const class Angle &a) ;

    operator double () { return radiant; }

};

class Grade : public Angle {

public:
 	
 	Grade () : Angle() {}
 	
 	Grade ( const double & grade ) ;
	Grade ( double gradi, double primi, double secondi ) ;
 	Grade ( const Radiant & r ) ;
 	Grade ( const Angle & a ) ;
 	Grade ( const Grade & g ) ;

	Grade &operator = ( const double &v);
	Grade &operator = ( const Radiant &v) ;
	Grade &operator = ( const Angle &a) ;
	Grade &operator = ( const Grade &a) ;

    operator double () { return toGrade(); }

};

typedef  Grade Longitude;

class Latitude : public Grade {

public:

 	Latitude ( const double & grade ) :
        Grade ( grade ) {}
 	Latitude ( const Radiant & r ) :
        Grade ( r ) {}
 	Latitude ( const Grade & g ) :
        Grade ( g ) {}

	Latitude &operator = ( const double &v) {

        setGrade ( v );
        return *this;

	}
	Latitude &operator = ( const Radiant &v) {
        radiant = v.radiant;
        return *this;
	}

	Latitude &operator = ( const Grade &v) {
        radiant = v.radiant;
        return *this;
	}

    operator double () {
        double g = toGrade();
        return g ;
    }

};

class AscensioneRetta : public Grade {

public:

 	AscensioneRetta ( const Radiant & r ) :
        Grade ( r ) {}
 	AscensioneRetta ( const Grade & g ) :
        Grade ( g ) {}

	AscensioneRetta &operator = ( const Radiant &v) {
        radiant = v.radiant;
        return *this;
	}

	AscensioneRetta &operator = ( const Grade &v) {
        radiant = v.radiant;
        return *this;
	}

    operator double () {
        double g = 24 * toGrade() / 360;
        return g ;
    }

    operator std::string () {
        
        return toGradeHMS ();

    }


};

class Declinazione : public Grade {

public:

 	Declinazione ( const Radiant & r ) :
        Grade ( r ) {}
 	Declinazione ( const Grade & g ) :
        Grade ( g ) {}

	Declinazione &operator = ( const Radiant &v) {
        radiant = v.radiant;
        return *this;
	}

	Declinazione &operator = ( const Grade &v) {
        radiant = v.radiant;
        return *this;
	}

    operator double () {
        double g = toGrade() ;
        return g ;
    }

    operator std::string () {
        
        return toGradeGPS ();

    }


};



#endif	// if _angle_
