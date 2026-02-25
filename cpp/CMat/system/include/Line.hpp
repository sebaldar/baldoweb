#ifndef _line_
#define _line_


#include <string>


#ifndef _compon_
	#include <componenti.hpp>
#endif

class LINEA : public COMPONENTE
{

public:


	std::string item;

	class NOZZLE * in;
	class NOZZLE * out;

	LINEA( const std::string & ident ) :
		COMPONENTE( ident ),
		in ( nullptr ),
		out ( nullptr ) {}
	
	~LINEA() {}

	
};



#endif
