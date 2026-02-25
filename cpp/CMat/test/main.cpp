#include <stdlib.h>

#include <iostream>
#include <math.h>

#ifndef _funzione_
   #include <funzione.hpp>
#endif

#ifndef _variable_
   #include <variabile.hpp>
#endif

#ifndef _cmat_
   #include <cmat.hpp>
#endif



class Test  
{

public:
	
	Variabile x1, x2, x3;
	
	Test () : f1(*this), f2(*this), f3(*this) {
	}
	
	~Test () {
	}

	struct F1 : public Funzione {
		Test &test;
		F1( Test &t ) : Funzione(), test(t) {}
		double eval() {
			return pow(test.x1,2) - test.x2 - 1 + test.x3;
		}
	}f1 ;

	struct F2 : public Funzione {
		Test &test;
		F2( Test &t ) : Funzione(), test(t) {}
		double eval() {
			return test.x1 + 4*test.x3 ;
		}
	}f2 ;

	struct F3 : public Funzione {
		Test &test;
		F3( Test &t ) : Funzione(), test(t) {}
		double eval() {
			return test.x3*(test.x2+1) -4*test.x2;
		}
	}f3 ;
	
	void execute () {
		
		Math < Funzione, Variabile, double > mat;
		mat.verbose = false;
		
		// genera il vettore delle variabili e quello delle funzioni
		std::vector< Variabile *> v_var;
		std::vector< Funzione * > v_funz;
		
		v_funz.push_back( &f1 );
		v_funz.push_back( &f2 );
		v_funz.push_back( &f3 );
		
		v_var.push_back( &x1 );
		v_var.push_back( &x2 );
		v_var.push_back( &x3 );

		try {
			mat.execute ( v_var, v_funz );
		}
		catch ( const std::string & err ) {
			std::cerr << err << std::endl;
		}
	
	}


} ;


int main (int argc, char *argv[])
{


	Test test;
	test.execute();

	std::cout << "x1=" << test.x1 << " x=" << test.x2 << " x3=" << test.x3 << std::endl;

	return (0);
	
}
