
#include <iostream>
#include <sstream>

#include <cstdlib>

#ifndef _field_h
	#include <field.hpp>
#endif


TFIELD::TAsInteger::TAsInteger ( TFIELD * _f) :
	f ( _f )
{
}

TFIELD::TAsInteger::operator int ()
{
	std::string::iterator it;
	for ( it = f->value.begin() ; it < f->value.end(); it++ )
		if ( !isdigit ( *it ) ) return 0 ;
	return atoi( f->value.c_str() ) ;
}

TFIELD::TAsBool::TAsBool ( TFIELD * _f) :
	f ( _f )
{
}

TFIELD::TAsBool::operator bool ()
{
	std::string::iterator it;
	for ( it = f->value.begin() ; it < f->value.end(); it++ )
		if ( !isdigit ( *it ) ) return 0 ;
	return !(f->value == "0");
}

TFIELD::TAsString::TAsString ( TFIELD * _f) :
	f ( _f )
{
}

TFIELD::TAsString::operator std::string ()
{
	return f->value ;
}

TFIELD::TAsFloat::TAsFloat ( TFIELD * _f) :
	f ( _f )
{
}

TFIELD::TAsFloat::operator float ()
{

	unsigned int dot_count = 0;
	auto it = f->value.begin() ;

    if ( it < f->value.end() && ( *it == '-' || *it == '+' ) ) {
        it++;
    }

	for ( ; it < f->value.end(); it++ )
		if ( !isdigit ( *it ) ) {
			if ( *it != '.' || dot_count > 1 )
				return 0 ;
			else
				dot_count++;
		}

	return atof( f->value.c_str() ) ;

}

TFIELD::TFIELD ( ) :
	value ( "" ), AsFloat ( this ), AsInteger ( this ), AsString ( this ), AsBool( this)
{
}

TFIELD::TFIELD ( std::string a_value ) :
	value ( a_value ), AsFloat ( this ), AsInteger ( this ), AsString ( this ), AsBool( this)
{
}

TFIELD::TFIELD ( const char * a_value ) :
	value ( a_value ), AsFloat ( this ), AsInteger ( this ), AsString ( this ), AsBool( this)
{
}

TFIELD::TFIELD ( int a_value ) :
	AsFloat ( this ), AsInteger ( this ), AsString ( this ), AsBool( this)
{

	std::stringstream ss ( std::stringstream::in | std::stringstream::out);

	ss << a_value ;

	ss >> value;

}

TFIELD::TFIELD ( float a_value ) :
	AsFloat ( this ), AsInteger ( this ), AsString ( this ), AsBool( this)
{

	std::stringstream ss ( std::stringstream::in | std::stringstream::out);

	ss << a_value ;

	ss >> value;

}

TFIELD::TFIELD ( bool a_value ) :
	value( a_value ? "1" : "0"),AsFloat ( this ), AsInteger ( this ), AsString ( this ), AsBool( this)
{
}

TFIELD & TFIELD::operator = ( int a_value )
{
	std::stringstream ss ( std::stringstream::in | std::stringstream::out);

	ss << a_value ;

	ss >> value;

	return *this ;

}

TFIELD::operator std::string ()
{

	return value ;

}
