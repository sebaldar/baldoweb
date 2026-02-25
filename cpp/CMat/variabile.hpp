#ifndef _variable_
#define _variable_

#include <iostream>
#include <sstream>


class Variabile {

public:
	std::string name ;
private:

	const double ZERO ;

	bool useRange ;
	

public:

	// la variabile e' fissa o puo' cambiare
	bool fix ;
	double val;             // valore

	// intervallo della variabile
	double min ;
	double max ;

	double Val (  ) ;

	void setMin( double _min ) ;
	void setMax( double _max ) ;
	void setRange ( double _min, double _max ) {
		min = _min;
		max = _max;
	}

	double getMin(  ) ;
	double getMax( ) ;

   /* costruttori e casting */
	Variabile() ;
	Variabile( double v ) ;
	Variabile ( const char * nome_var );
	Variabile ( const std::string & nome_var );
	Variabile(const Variabile &v) ;

	double &GetRif () {
		return val;
	}

	void Fix ();
	void UnFix ();
	bool IsFix() const ;

	Variabile &operator = (Variabile &v);
	Variabile &operator = (double f);
	Variabile &operator += (double f);

	operator double () const;

	void Rand () ;
	void Med () ;

	const char *GetName () const ;

	// normalizza la variabile nell'intervallo min .. max
	void Normalize ();
	void DeNormalize ();

	
};


#endif	// if _variable_
