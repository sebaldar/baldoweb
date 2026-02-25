
#ifndef __CONFIG__
#define __CONFIG__
#include <json.hpp>
#include <sstream>

	

namespace config {

	void config_data () ;

	struct {
		
		std::string DATADIR = std::string(getenv("HOME")) + "/PLANETARIUM/webroot/data/";
		
		struct {
			std::string host = getenv("DB_HOST");
			std::string user = getenv("DB_USER");
			std::string psw = getenv("DB_PASS");
			std::string db = getenv("DB_NAME");
		} database;
	
	} DATA;
	
}	// namespace config 


#endif
