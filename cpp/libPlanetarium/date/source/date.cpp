#include <fstream>
#include <sstream>
#include <math.h>       /* floor */

#include <date.hpp>
// #include <angolo.hpp>

const double Date::sec_in_a_day = 24 * 60 * 60 ;

const double JCentury::JD2000 = 2451545.0;
const double JCentury::JULIANCENTURY =  36525.0;	// days in a century
const double JCentury::JULIANMILLENIUN = 365250.0 ;	// days in a milleniun

Date::Date (  )
{

	time_t rawtime;
	time ( &rawtime );

	tm *t = gmtime ( &rawtime );

	JD = julianDay ( t );

	timer.start();

}

Date::Date(const std::string & str_date) {
    
    std::string str = formatDate(str_date);

    tm t = {}; // inizializza a zero
    if (strptime(str.c_str(), "%d-%m-%Y %H:%M:%S", &t)) {
        JD = julianDay(&t);
    } else {
        throw std::runtime_error("Formato data non valido: " + str_date);
    }

    timer.start();
}

Date::Date ( double jd ) : JD( jd )
{

	timer.start();

}

Date::~Date()
{

}

double Date::deltaT ( ) const
{

	return deltaT ( year(), month() );
	
}

double Date::deltaT ( short year, short month ) const
{

// fonte http://eclipse.gsfc.nasa.gov/LEcat5/deltatpoly.html

	double y = year + (month - 0.5)/12 ;
	double c = -0.000012932 * pow( (y - 1955), 2 );

	if ( year <= -500 ) {
		double u = (year-1820)/100;
		return -20 + 32 * pow( u, 2 ) + c;
	}
	else if ( year > -500 && year <= 500 ) {
		double u = y/100 ;
		return 10583.6 - 1014.41 * u + 33.78311 * pow(u, 2) - 5.952053 * pow(u, 3)
			- 0.1798452 * pow( u, 4) + 0.022174192 * pow( u, 5 ) + 0.0090316521 * pow(u, 6)  + c;
	}
	else if ( year > 500 && year <= 1600 ) {
		double u = (y-1000)/100;
		return 1574.2 - 556.01 * u + 71.23472 * pow(u, 2) + 0.319781 * pow(u, 3)
		- 0.8503463 * pow(u,4) - 0.005050998 * pow(u, 5) + 0.0083572073 * pow(u, 6) + c;
	}
	else if ( year > 1600 && year <= 1700 ) {
		double t = y - 1600;
		return 120 - 0.9808 * t - 0.01532 * pow(t, 2) + pow(t, 3) / 7129 + c;
	}
	else if ( year > 1700 && year <= 1800 ) {
		double t = y - 1700;
		return 8.83 + 0.1603 * t - 0.0059285 * pow(t, 2) + 0.00013336 * pow(t, 3) - pow(t, 4) / 1174000 + c;
	}
	else if ( year > 1800 && year <= 1860 ) {
		double t =  y - 1800;
		return 13.72 - 0.332447 * t + 0.0068612 * pow(t, 2) + 0.0041116 * pow(t, 3) - 0.00037436 * pow(t, 4)
			+ 0.0000121272 * pow(t, 5) - 0.0000001699 * pow(t, 6) + 0.000000000875 * pow(t, 7) + c;
	}
	else if ( year > 1860 && year <= 1900 ) {
		double t =  y - 1860;
		return 7.62 + 0.5737 * t - 0.251754 * pow(t, 2) + 0.01680668 * pow(t, 3)
			-0.0004473624 * pow(t, 4) + pow(t, 5) / 233174 + c;
	}
	else if ( year > 1900 && year <= 1920 ) {
		double t =  y - 1900;
		return -2.79 + 1.494119 * t - 0.0598939 * pow(t, 2) + 0.0061966 * pow(t, 3) - 0.000197 * pow(t, 4) + c;
	}
	else if ( year > 1920 && year <= 1941 ) {
		double t =  y - 1920;
		return 21.20 + 0.84493*t - 0.076100 * pow(t,2) + 0.0020936 * pow(t,3) + c;
	}
	else if ( year > 1941 && year <= 1961 ) {
		double t =  y - 1950;
		return 29.07 + 0.407*t - pow(t, 2)/233 + pow(t,3) / 2547;
	}
	else if ( year > 1941 && year <= 1955 ) {
		double t =  y - 1950;
		return 29.07 + 0.407*t - pow(t, 2)/233 + pow(t,3) / 2547 + c;
	}
	else if ( year > 1955 && year <= 1986 ) {
		double t =  y - 1975;
		return 45.45 + 1.067*t - pow(t,2)/260 - pow(t,3) / 718;
	}
	else if ( year > 1986 && year <= 2005 ) {
		double t =  y - 2000;
		return 63.86 + 0.3345 * t - 0.060374 * pow(t,2) + 0.0017275 * pow(t, 3) + 0.000651814 * pow(t,4)
		+ 0.00002373599 * pow(t, 5);
	}
	else if ( year > 2005 && year <= 2050 ) {
		double t =  y - 2000;
		return 62.92 + 0.32217 * t + 0.005589 * pow(t,2) + c;
	}
	else if ( year > 2050 && year <= 2150 ) {
		return -20 + 32 * pow( ( (y-1820)/100 ), 2) - 0.5628 * (2150 - y) + c;
	}
	else  {  // if ( year > 2150 ) {
		double u = (year-1820)/100;
		return -20 + 32 * pow( u, 2 )  + c;
	}


}

time_t Date::mktimeUTC( struct tm* timeinfo ) const
{
    // *** enter in UTC mode
    char * oldTZ = getenv("TZ");
    putenv(  const_cast<char *> ("TZ=UTC") );
    tzset();
    // ***

    time_t ret = mktime ( timeinfo );

    // *** Restore previous TZ
    if(oldTZ == NULL)
    {
        putenv( const_cast<char *> ("TZ=") );
    }
    else
    {
        char buff[255];
        sprintf(buff,"TZ=%s",oldTZ);
        putenv(buff);
    }
    tzset();
    // ***

    return ret;

}

std::string Date::formatDate ( const std::string & str_date ) const
{

    // il formato è "%d-%m-%Y %H:%M:%S"
    std::stringstream ss;
    ss << str_date;
    std::string g, h;

    ss >> g;
    ss >> h;

    // se non ora imposta a mezzogiorno
    if ( h == "" )
        h = "12:00:00";

    return g + " " + h;

}

double Date::julianDay (  ) const
{

	return JD	;


}

double Date::julianDay ( tm * timeinfo ) const
{

	double day = timeinfo->tm_mday;
	short month = timeinfo->tm_mon + 1;
	short year = timeinfo->tm_year + 1900;

	short hour = timeinfo->tm_hour;
	short mn = timeinfo->tm_min;
	short sec = timeinfo->tm_sec ;

	double fract = ( hour * 60 * 60 + mn * 60 + sec ) / sec_in_a_day;

	day += fract;

	if ( month == 1 || month == 2 ) {
		year -= 1;
		month += 12;
	}

	double A = floor( year / 100 ),
		B = 2 - A + floor( A / 4 );

	return floor( 365.25 * ( year + 4716 ) ) + floor( 30.6001 * ( month + 1 ) ) + day + B - 1524.5	;


}

std::string Date::toDate ( )  const
{

	double jd = now();

	return toDate ( jd );


}

std::string Date::toDate ( double jd )  const
{

	Calendar cal ( jd );
	short d = cal.day;
	short m = cal.month;
	short y = cal.year;
	short h = cal.hour;
	short mn = cal.minute;
	short s = cal.second;

	std::stringstream ss ;
	ss << d << "-" << m << "-" << y << " " << h << ":" << mn << ":" << s ;

	return ss.str() ;

}

std::string Date::toDateMSQL ( )  const
{

	double jd = now();

	return toDateMSQL ( jd );


}

std::string Date::toDateMSQL ( double jd )  const
{

	Calendar cal ( jd );
	short d = cal.day;
	short m = cal.month;
	short y = cal.year;
	short h = cal.hour;
	short mn = cal.minute;
	short s = cal.second;

	std::stringstream ss ;
	ss << y << "-" << m << "-" << d << " " << h << ":" << mn << ":" << s ;

	return ss.str() ;

}

short Date::year ( ) const
{

	double jd = now();

	return year ( jd );

}

short Date::year ( double jd ) const
{

	Calendar cal ( jd );
	short y = cal.year;

	return y ;

}

short Date::month (  ) const
{

	double jd = now();

	return month ( jd );

}

short Date::month ( double jd ) const
{

	Calendar cal ( jd );
	short m = cal.month;

	return m ;

}

short Date::day (  ) const
{

	double jd = now();

	return month ( jd );

}

short Date::day ( double jd ) const
{

	Calendar cal ( jd );
	short d = cal.day;

	return d ;

}

short Date::dayoftheweek (  ) const
{

	double jd = now();

	return dayoftheweek ( jd );

}

short Date::dayoftheweek ( double jd ) const
{

    std::string str = toDate ( jd ) ;

    return dayoftheweek( str );

}

double Date::JDatMidnight () const
{

	Calendar cal ( JD );
	short d = cal.day;
	short m = cal.month;
	short y = cal.year;
	short h = 0;
	short mn = 0;
	short s = 0;

	std::stringstream ss ;
	ss << d << "-" << m << "-" << y << " " << h << ":" << mn << ":" << s ;


  std::string str = formatDate( ss.str() );

  tm t = {}; // inizializza a zero
  if (strptime(str.c_str(), "%d-%m-%Y %H:%M:%S", &t)) {
		return julianDay ( &t );
  } else {
      throw std::runtime_error("Formato data non valido: " + str);
  }

}

short Date::dayoftheweek ( const std::string & day ) const
{


    std::string str = formatDate( day );

  tm t = {}; // inizializza a zero
  if ( strptime( str.c_str(), "%d-%m-%Y %H:%M:%S", &t ) != nullptr ) {

    time_t t_t = mktime( &t );
    tm * timeinfo = localtime (&t_t);
    t_t = mktimeUTC ( timeinfo );
    timeinfo = localtime (&t_t);

//        char * s = asctime(timeinfo);
//    std::cout << "Current local time and date: " << asctime(timeinfo) << " " << day << std::endl;
    return timeinfo->tm_wday; // Sunday=0, Monday=1, etc.

  }

  return -1;

}

Date::Calendar::Calendar ( double jd )
{

	double Z = floor( jd + 0.5 );
	double F = ( jd + 0.5 ) - floor( jd + 0.5 );

	double A;
	if ( Z < 2299161 ) {
			A = Z;
	}
	else {

		double alfa = floor ( ( Z - 1867216.25 ) / 36524.25 );
		A = Z + 1 + alfa - floor( alfa / 4 ) ;

	}

	double B = A + 1524;
	double C = floor( (B - 122.1 ) / 365.25 );
	double D = floor( 365.25 * C ) ;
	double E = floor( (B - D ) / 30.6001 ) ;

	double d1 = B - D - floor( 30.6001 * E ) + F;

	day = floor( d1 );
	double t = sec_in_a_day * ( d1 - day );

	hour = floor ( t / 3600 ) ;
	minute = ( t - hour * 3600 ) / 60;
	second = ( t - hour * 3600 - minute * 60 )  ;

	if ( E < 14 )
		month = E - 1 ;
	else
		month = E - 13 ;

	if ( month > 2 )
		year = C - 4716 ;
	else
		year = C - 4715 ;

}

Date::Calendar Date::calendar ( double jd ) const
{

	return Calendar ( jd );

}

double Date::now () const {
	return now ( 1 );
}

double Date::now ( double times ) const {
	return JD + times * timer.secs_elapsed() / sec_in_a_day;
}

double Date::hours () const
{

	double jd = now();
	Calendar c ( jd );
	double h = c.hour;
	double m = c.minute;
	double s = c.second;
	
	return h + m / 60 + s / 3600;
	
}

double Date::hours ( double times ) const
{

	double jd = now(times);
	Calendar c ( jd );
	double h = c.hour;
	double m = c.minute;
	double s = c.second;
	
	return h + m / 60 + s / 3600;
	
}

// implementazione della classe JDay
JDay::JDay ( double jd ) :
	Date ( jd )
{
}

JDay::JDay ( const Date & date ) :
	Date ( date.julianDay() )
{
}


JDay::JDay ( const std::string & str_date ) :
	Date ( str_date )
{

}

JDay &JDay::operator = ( double jd )
{
	JD = jd;
	timer.start();

	return * this;
}

JDay &JDay::operator = ( const Date & date )
{
	JD = date.julianDay();
	timer = date.timer;

	return * this;
}

std::string JDay::j2000 () {

	JDay jd2000 = JCentury::JD2000;
	return jd2000.toDate ();

}

// implementazione della classe JCentury

JCentury::JCentury ( double jc ) :
	Date ( jc * 36525.0 + JD2000 )
{
}

JCentury::JCentury ( const Date & date ) :
	Date ( date.julianDay() )
{

}

JCentury::JCentury ( const std::string & str_date ) :
	Date ( str_date )
{

}

JCentury &JCentury::operator = ( const double jc )
{
	JD = jc * 36525.0 + JD2000 ;
	return * this;
}

JCentury &JCentury::operator = ( const Date & date )
{
	JD = date.julianDay();
	return * this;
}




DT::DT ( ) :
	Date () 
{
}	
	

DT::DT ( const UT & ut ) 
{
	*this = ut;
}

DT &DT::operator = ( const UT &ut) 
{
		JD = ut.JD + ut.deltaT() / Date::sec_in_a_day;
		timer = ut.timer;
		return *this;
}

UT::UT ( ) :
	Date () 
{
}	
	

UT::UT ( const DT & dt ) 
{
	*this = dt;
}


UT &UT::operator = ( const DT &dt) 
{
		JD = dt.JD - dt.deltaT() / Date::sec_in_a_day;
		timer = dt.timer;
		return *this;
}

	
SideralTime::SideralTime ( ) 
{
}

SideralTime::SideralTime (  const Date & date ) :
	Date ( date.julianDay() )
{
}

SideralTime &SideralTime::operator = ( const Date & date )
{
	JD = date.julianDay();
	timer = date.timer;

	return * this;
}

	
double SideralTime::teta0 () const
{
	// converte JD al mean sideral time at Greenwhich at 0h UT
	// espressa in gradi e decimali
	double jd0 = JDatMidnight ();
	double T = ( jd0 - 2451545.0 ) / 36525 ;

		

	return 
		100.46061837 +
		36000.770053608 * T +
		0.000387933 * T * T -
		T * T * T / 38710000;

/*		
	double s0 = 6 * 3600 + 41 * 60 + 50.54841 ;
	double s = 8640184.812866 * T +	0.093104 * T * T -	0.0000062 * T * T * T ;
	// normalizziamo i secondi
	double sn = s - ( floor( s / 86400 ) ) * 86400;
	
	double  h = ( s0 + sn ) / 3600 ;
	
		
		std::string LOGFILE = "/tmp/server.log";
		std::fstream srvlog ( LOGFILE.c_str(),  std::fstream::in | std::fstream::out | std::fstream::app );
		srvlog.precision(12);
		srvlog << "T " << T << " s=" << s << " h " << h << std::endl ;

	return Grade( h * 15 );
*/


}

double SideralTime::teta () const
{
	// TODO
	return 0;
}

