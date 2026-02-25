#ifndef resource_hpp
#define resource_hpp

#include <iostream>
#include <vector>

#include "tmysql.hpp"

class Resource  : public TQuery {

protected :

	std::string documentRoot ;

public :

	Resource ( const std::string & document_root   );
	virtual ~Resource ( );


};



#endif
