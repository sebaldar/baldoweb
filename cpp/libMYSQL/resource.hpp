#include <iostream>
#include <vector>

#include "tmysql.hpp"

class Resource  : public TQuery {

protected :

	std::string documentRoot ;
	static TConnection * stat_connection ;

public :

	Resource ( const std::string & document_root   );
	virtual ~Resource ( );


};

