#include <fstream>
#include <sstream>
#include <json.hpp>
#include "../rapidjson-master/include/rapidjson/document.h"
#include "../rapidjson-master/include/rapidjson/stringbuffer.h"
#include "../rapidjson-master/include/rapidjson/writer.h"

/*
 * //! Type of JSON value
enum Type {
    kNullType = 0,      //!< null
    kFalseType = 1,     //!< false
    kTrueType = 2,      //!< true
    kObjectType = 3,    //!< object
    kArrayType = 4,     //!< array
    kStringType = 5,    //!< string
    kNumberType = 6     //!< number
};
*/

JSON::JSON (  ) :
	doc ( new rapidjson::Document )
{
}

JSON::JSON ( const std::string & str) :
	JSON ()
{
	loadFromString( str );
}

JSON::~JSON (  )
{
	delete reinterpret_cast<rapidjson::Document *> (doc);
}

void JSON::loadFromFile ( const std::string & file_name )
{

	std::stringstream ss ;

	std::ifstream ifs ( file_name.c_str(), std::ifstream::in );

  	if ( !ifs.good() ) {

		std::stringstream ss ;
		ss << "Errore di lettura del file " << file_name << std::endl;
		ss << " good()=" << ifs.good();
		ss << " eof()=" << ifs.eof();
		ss << " fail()=" << ifs.fail();
		ss << " bad()=" << ifs.bad();
		ss << std::endl;

		ifs.close();

		throw ( ss.str() ) ;

	}

  	while ( !ifs.eof () ) {

		std::string str;

		std::getline( ifs, str ) ;
		ss << str ;

	}

	ifs.close();

	rapidjson::Document &document = * reinterpret_cast<rapidjson::Document *> (doc) ;
	document.Parse(ss.str().c_str());

}

void JSON::loadFromString ( const std::string & str )
{
	rapidjson::Document &document = * reinterpret_cast<rapidjson::Document *> (doc) ;
	document.Parse( str.c_str() );
}

std::vector< std::string > JSON::array( const std::string & ar )
{
	// array nel formato [ value1, value2, ... ]
	rapidjson::Document &document = * reinterpret_cast<rapidjson::Document *> (doc) ;
	rapidjson::Value& value = document[ar.c_str()];

	std::vector < std::string > v;
	if ( value.IsArray() ) {

		for ( rapidjson::SizeType i = 0; i < value.Size(); i++ ) { // Uses SizeType instead of size_t

			switch ( value[i].GetType() ) {

				case rapidjson::kNullType :      //!< null
					v.push_back("");
				break;
				case rapidjson::kFalseType :    //!< false
					v.push_back("0");
				break;

				case rapidjson::kTrueType   :   //!< true
					v.push_back("1");
				break;

				case rapidjson::kObjectType  : //!< object
				case rapidjson::kArrayType :  //!< array

				{

					rapidjson::StringBuffer buffer;
					rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
					value[i].Accept(writer);

					v.push_back( buffer.GetString() );

				}

				break;

				case rapidjson::kStringType :   //!< string
					v.push_back( value[i].GetString() );
				break;

				case  rapidjson::kNumberType :     //!< number
					std::stringstream ss ;
					ss << value[i].GetDouble();
					v.push_back( ss.str() );
				break;

			}

		}

	}

	return v;

}

std::string JSON::operator [] ( const std::string & str )
{
	return value( str );
}

std::string JSON::value ( const std::string & str )
{

	rapidjson::Document &document = * reinterpret_cast<rapidjson::Document *> (doc) ;

	if ( !document.HasMember( str.c_str() ) )
		return "";

	rapidjson::Value& value = document[str.c_str()];

	switch ( value.GetType() ) {

		case rapidjson::kNullType :      //!< null
			return "";

		case rapidjson::kFalseType :    //!< false
			return "0";

		case rapidjson::kTrueType   :   //!< true
			return "1";

		case rapidjson::kObjectType  : //!< object
		case rapidjson::kArrayType :  //!< array

		{
			rapidjson::StringBuffer buffer;
			rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
			value.Accept(writer);

			return  buffer.GetString() ;
		}

		case rapidjson::kStringType :   //!< string
			return value.GetString() ;

		case  rapidjson::kNumberType :     //!< number
		{
			std::stringstream ss ;
			ss << value.GetDouble();
			return ss.str();
		}

		default :
			return "";

	}

}
