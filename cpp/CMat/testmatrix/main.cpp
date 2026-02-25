#include <iostream>
#include <fstream>

#include <math.h>

#ifndef _matrix_
	#include <matrix.hpp>
#endif

// per Trim
#ifndef until_h
	#include "Utils.h"
#endif



int main (int argc, char *argv[])
{
/*

	B_MATRIX m ( 3, 3 );
	VECTOR v_ec ( 3 );
	VECTOR v_eq ( 3 );
	
	m(0,0)=1;
	m(0,1)=1;
	m(0,2)=0;

	m(1,0)=1;
	m(1,1)=0;
	m(1,2)=1;

	m(2,0)=1;
	m(2,1)=1;
	m(2,2)=-1;

	v_ec[0] = 1;
	v_ec[1] = 1;
	v_ec[2] = 1;
	
	v_eq = m * v_ec;
*/

	float epsilon = 25 * 3.14 / 180;
	float x_ec = 0, y_ec = 1, z_ec=0;
	
	B_MATRIX m ( 3, 3 );
	VECTOR v_ec ( 3 );
	VECTOR v_eq ( 3 );
	
	m(0,0)=1;
	m(0,1)=0;
	m(0,2)=0;

	m(1,0)=0;
	m(1,1)=cos(epsilon);
	m(1,2)=-sin(epsilon);

	m(2,0)=0;
	m(2,1)=sin(epsilon);
	m(2,2)=cos(epsilon);

	v_ec[0] = x_ec;
	v_ec[1] = y_ec;
	v_ec[2] = z_ec;
	
	v_eq = m * v_ec;

	std::cout << v_eq[0] << " " << v_eq[1] << " " << v_eq[2] << std::endl;
	return (0);

}

