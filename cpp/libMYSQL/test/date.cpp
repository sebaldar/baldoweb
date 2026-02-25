			std::string dc = data->ref_date.toDate(jd);
			std::vector< std::string> p ;
//size_t split ( const std::string& input, const std::string& delimiter, std::vector<std::string> &v );
			utils::split( dc, " ", p );
			std::string dd = p[0];
			std::string tt = p[1];
			
			std::vector< std::string> r;
			utils::split ( dd, "-", r );
			std::string d = r[0], m = r[1], y = r[2];
			
			ss.precision(10);
			ss << "Date set to ";
			ss << data->ref_date.toDate(jd) << " jd " << jd <<  std::endl;
			
			std::stringstream the_data;
			the_data <<
				"<input id=\"date\" value=\""  <<
				y << "-" << m << "-" << d << "\" />" <<
				"<input id=\"time\" value=\""  <<
				tt << "\" />" 
			;
				

			std::stringstream xml;
		xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<  
			"<results>" << 
				"<terminal>" << ss.str() << the_data.str()<< "</terminal>" <<
//				"<data>" << the_data.str() << "</data>" <<
			"</results>";
