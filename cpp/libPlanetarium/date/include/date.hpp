#ifndef _date_
#define _date_

#include <time.h>       /* time_t, struct tm, difftime, time, mktime */
#include <iostream>
#include <timer.hpp>

class Date  {

    struct Calendar {

        short day, month, year;
        short hour, minute, second;

        Calendar () {}
        Calendar ( double js ) ;
        Calendar ( short d, short m, short y, short h, short _m, short s ) :
            day(d), month(m), year(y), hour(h), minute(_m), second(s) {}

    };

    Calendar calendar ( double js ) const ;
    std::string formatDate ( const std::string & str_date ) const;
    time_t mktimeUTC(struct tm* timeinfo) const ;


protected :

	
public:

	Timer timer;

	double JD ;

	static const double sec_in_a_day ;
	
	Date ( ) ;
	Date ( const std::string & str_date ) ;	// nel formato "04-12-2013 15:03:20")
	Date ( double julianDay ) ;

	virtual ~Date() ;

	Date &operator = ( const Date &v) {
		JD = v.JD;
		timer = v.timer;
		return *this;
	}
	
	Date &operator = ( double jd ) {
		JD = jd;
		timer.start();
		return *this;
	}

    operator std::string () { 
		return toDate();
	};

	void resume ( double simulation ) {
		JD = now( simulation );
		timer.start();
	}
	
	virtual std::string toDate ( ) const ;
	std::string toDate ( double jd )  const;
	std::string toDateMSQL ( )  const;
	std::string toDateMSQL ( double jd )  const;
	
	double julianDay ( tm * timeinfo ) const ;
	double julianDay (  ) const ;

	short year (  ) const ;
	short year ( double jd ) const ;
	short month (  ) const ;
	short month ( double jd ) const ;
	short day (  ) const ;
	short day ( double jd ) const ;
	short dayoftheweek (  ) const ;
	short dayoftheweek ( double jd ) const ;
	short dayoftheweek ( const std::string & day ) const ;

	double now () const ;	// ritorna jd + il tempo trascorto dall'istanza
	
	double hours () const ;

	double now ( double times ) const ;	// ritorna jd + il tempo trascorto dall'istanza moltiplicato per times
	
	double hours ( double times ) const ;

	double deltaT ( ) const ;
	double deltaT ( short year, short month ) const ;
	
	double JDatMidnight () const ;

};

class DT : public Date {

	public:

	DT ( ) ;
	DT ( const std::string & str_date ) : Date( str_date ) {}	// nel formato "2013-12-04 15:03:20")
	DT ( double julianDay ) : Date( julianDay ) {}
	DT ( const class UT & ut );
	DT ( const class Date & dt ) {
		JD = dt.JD ;
		timer = dt.timer;
	}

	DT &operator = ( const class  UT &ut) ;

};

class UT : public Date {

	public:

	UT ( ) ;
	UT ( const std::string & str_date ) : Date( str_date ) {}	// nel formato "2013-12-04 15:03:20")
	UT ( double julianDay ) : Date( julianDay ) {}
	UT ( const DT & dt ) ;
	UT ( const class Date & dt ) {
		JD = dt.JD ;
		timer = dt.timer;
	}


	UT &operator = ( const DT &dt) ;
	UT &operator = ( double jd ) {
		JD = jd;
		timer.start();
		return *this;
	}

};

class JDay : public Date  {

public:

	JDay ( ) ;
	JDay ( double jd ) ;
	JDay ( const std::string & str_date ) ;	// nel formato "2013-12-04 15:03:20")
	JDay ( const Date & date ) ;

	JDay &operator = ( double jd );
	JDay &operator = ( const Date & date );

    operator double () { return JD; };

    std::string j2000 () ;

};

class JCentury : public Date  {


public:


    static const double JULIANCENTURY  ;	// days in a century
    static const double JULIANMILLENIUN  ;	// days in a milleniun
    static const double JD2000  ;

	JCentury ( ) ;
	JCentury ( double jc ) ;
	JCentury ( const std::string & str_date ) ;	// nel formato "2013-12-04 15:03:20")
	JCentury ( const Date & date ) ;
	JCentury ( const JDay & tjd )  {
        JD = tjd.julianDay();
	}

	JCentury &operator = ( double jc );
	JCentury &operator = ( const Date & date );
	JCentury &operator = ( const JDay & tjd ) {
        JD = tjd.julianDay();
        return *this;
	}

    operator double () {

		return ( JD - JD2000 ) / 36525.0 ;

	}

};

class SideralTime : public Date {
	
	public:
	
	SideralTime ( ) ;
	SideralTime (  const Date & date ) ;
	SideralTime ( const JDay & tjd )  {
        JD = tjd.julianDay();
	}
	SideralTime &operator = ( const Date & date );
	SideralTime &operator = ( const JDay & tjd ) {
        JD = tjd.julianDay();
        return *this;
	}
	
	double teta0 () const ;
	double teta () const ;

};


#endif	// if _date_
