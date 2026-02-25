#ifndef _field_
#define _field_


class TFIELD {

	class TAsInteger {

		TFIELD * f ;

	public :

		TAsInteger ( TFIELD * _f) ;

		operator int () ;

	} ;

	class TAsString {

		TFIELD * f ;

	public :

		TAsString ( TFIELD * _f) ;

		operator std::string () ;

	} ;

	class TAsFloat {

		TFIELD * f ;

	public :

		TAsFloat ( TFIELD * _f) ;

		operator float () ;

	} ;

	class TAsBool {

		TFIELD * f ;

	public :

		TAsBool ( TFIELD * _f) ;

		operator bool () ;

	} ;

	std::string value ;

public :

	TFIELD ( ) ;
	TFIELD ( std::string a_value ) ;

	TFIELD ( const char * a_value ) ;
	TFIELD ( int a_value ) ;
	TFIELD ( float a_value ) ;
	TFIELD ( bool a_value ) ;


	TFIELD & operator = ( int a_value );
	operator std::string () ;

	TAsFloat AsFloat ;
	TAsInteger AsInteger ;
	TAsString AsString ;
	TAsBool AsBool ;

} ;

 #endif
