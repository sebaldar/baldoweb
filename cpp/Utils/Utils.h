
#ifndef _utils_
#define _utils_

#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <vector>
#include <zlib.h>

#include <nlohmann/json.hpp>

#ifndef MIN
	#define MIN(x,y) ( x<=y ? x : y )
#endif

#ifndef MAX
	#define MAX(x,y) ( x>=y ? x : y )
#endif

using json = nlohmann::json;

namespace utils {

	std::string urldecode (   const std::string & input) ;
	bool is_utf8( const char * string );
	std::string file_get_contents ( const std::string & file_name ) ;
	void file_set_contents ( const std::string & file_name, const std::string & content  ) ;

/*
	separa la stringa di input iin accordo a delimiter e
	inserisce il risultato in v
 */
	size_t split ( const std::string& input, const std::string& delimiter, std::vector<std::string> &v );
	std::string trim ( const std::string &str );
	std::string getWord ( const std::string & str, size_t & i )  ;

	bool isNumber ( const char * number ) ;
	bool isNumber ( const std::string & number ) ;

	std::string & str_replace( const std::string & search, const std::string &replace, std::string & subject );
	
	std::string & toUpper ( std::string & str );
	std::string getValue ( const std::string &from_text, const std::string &name, const std::string &separatore );

	void replace ( const std::string & da_str, const std::string & a_str, std::string & nel_testo ) ;
	
	std::string getProcStatus();
  
  json listFilesInDirectory(const std::string& path) ;
  
  std::string compressToWebSocket(const std::string& str, int level = Z_BEST_COMPRESSION) ;  


}	// namespace utils 


std::string FloatToStr ( float f );
std::string IntToStr ( float i ) ;

bool isRegolarFile ( std::string nome_file );
bool FileExists ( const std::string & nome_file );


std::string Sostituisci ( std::string da_str, std::string a_str, std::string nel_testo );

std::string strHash ( std::string str );

int GetStrInBraket (const char *str) ;
int GetStrTraDoppiApicini(const char *str) ;
int GetStrTraApicini(const char *str) ;

char * LTrim (char * arg);
char * RTrim (char * arg);
char * DelBlk(char * arg);

std::string Trim ( const std::string & _testo );		// toglie spazi/tab/finelinea a sx e dx.

char * lLTrim (char * arg);
char * lRTrim (char * arg);
char * lDelBlk(char * arg);

int lstrncmp(const char * s1, const char * s2, int n);

bool fileExists ( const std::string & strFilename );

std::string getValue ( const std::string &from_text, const std::string &name, const std::string &separatore="&" );

// confronta due stringhe non considerando maiuscole/minuscole
int strncmp_no_case(const char * s1, const char * s2, int n);

char * lstrncpy(char * s1, char * s2, int n);
char * lstrncat(char * s1, char * s2, int n);
char * lStrChr(char * sel, char * arg, char chr);


#if defined(__cplusplus) && __cplusplus
 extern "C" {
#endif

	char * strupr(char *string);
	char * strlwr(char *string);

#if defined(__cplusplus) && __cplusplus
 }
#endif

std::string trim ( const std::string &str );
std::string toLower ( std::string str ) ;
std::string toUower ( std::string str ) ;


// =======================================
// C++Wrapper around popen
//   that recognaizes invalid command_line
// ---------------------------------------
//   Alex Vinokur
//     mailto:alexvn@go.to
//     http://go.to/alexvn
// ---------------------------------------
//   2002-10-22
// =======================================


// ==============================================
// Windows 2000 Professional
// MinGW 2.0.0.-2
// gcc/g++ version 3.2 (mingw special 20020817-1)
// ==============================================

// ==================
// =====================================
#define FATAL_MSG(msg) \
        std::cout << msg \
             << " : " \
             << (strerror (errno)) \
             << " [ " \
             << __FILE__ \
             << ", #" \
             << __LINE__ \
             << " ]" \
             << std::endl

#define ERROR_MSG(msg) \
        std::cout << msg \
             << " : " \
             << " [ " \
             << __FILE__ \
             << ", #" \
             << __LINE__ \
             << " ]" \
             << std::endl


// =====================================
class p_open {

	bool _popen ( const std::string&   command_line_i );

public:

	std::vector<std::string> out_result_o;
	std::vector<std::string> err_result_o;

	std::string out ();

	p_open ( );
	p_open ( std::string & command_line );
	p_open ( const char * command_line );

};

void log_file ( std::string & msg );
void log_file ( float msg );

 #endif
