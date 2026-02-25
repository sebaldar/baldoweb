#include <sstream>

#ifndef AsserzioniH
//   #include <Asserzioni.h>
#endif	// #ifndef AsserzioniH

#ifndef _variable_
   #include <variabile.hpp>
#endif


/* Classe TVARIABLE */
Variabile::Variabile() :
	Variabile ( "" )
{
}

Variabile::Variabile ( double v ) :
	Variabile ( "" )
{
	val = v;
}


Variabile::Variabile ( const char * nome_var ) :
	name(nome_var),
	ZERO(1e-15),
	useRange( false ),
	fix(false),
	val(ZERO),
	min(-1),
	max(1)
{
}


Variabile::Variabile ( const std::string & nome_var ) :
	Variabile(nome_var.c_str())
{
}

Variabile::Variabile(const Variabile &v) :
    name(v.name),
    ZERO(v.ZERO),
    useRange(v.useRange),
    fix(v.fix),
    val(v.val),
    min(v.min),
    max(v.max)
{
}

Variabile &Variabile::operator = ( Variabile &v )
{

	name = v.name;
	val = v.val;
	fix = v.fix;
	min = v.min;
	max = v.max;
	useRange = v.useRange;

	return *this;
	
}

Variabile & Variabile::operator = (double v)
{

	if ( !useRange )
		val = v;
	else {

		val = ( v >= min && v <= max ) ?
			v :
			v < min ?
			min : max ;

	}

	return *this;

}

Variabile &Variabile::operator += (double v)
{

	if ( !useRange )
		val += v;
	else {

		val += ( v >= min && v <= max ) ?
			v :
			v < min ?
			min : max ;

	}

	return *this;

}

Variabile::operator double () const
{

	/*
	se è nell'intervallo min - max ritorna val
	* altrimenti ritorn o min o max
	*/ 
	return val;

}

double Variabile::Val (  ) {

	return double( *this );

}

void Variabile::setMin( double _min ) {
	min = _min;
//	useRange = true;
}

void Variabile::setMax( double _max ) {
	max = _max;
//	useRange = true;
}

double Variabile::getMin(  ) {
	return min ;
}

double Variabile::getMax( ) {
	return max ;
}

bool Variabile::IsFix() const
{
	return fix;
}

void Variabile::Fix()
{
	fix = true;
}

void Variabile::UnFix()
{
	fix = false;
}

void Variabile::Med ()
{
	// da implementare
	val = (min + max) / 2;
}

const char *Variabile::GetName ()const
{
	return name.c_str();
}

void Variabile::Rand ()
{
	// un numero casuale tra min e max
	val = min + (max - min) * ((double) rand() / RAND_MAX);

}

	// normalizza la variabile nell'intervallo min .. max
void Variabile::Normalize ()
{
	val = (val - min) / (max - min) ;
}

void Variabile::DeNormalize ()
{
	val = min + val * (max - min) ;
}
