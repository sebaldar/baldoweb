#include <cerrno>
#include <sstream>
#include <assert.h>
#include <cstring>
#include <algorithm>
#include <filesystem>


#include <unistd.h>

#ifndef until_h
	#include "Utils.h"
#endif

namespace fs = std::filesystem;


/* +++Date last modified: 05-Jul-1997 */

/*
**  Portable, public domain replacements for strupr() & strlwr() by Bob Stout
*/

#include <ctype.h>

std::string FloatToStr ( float f )
{

        std::stringstream ss (std::stringstream::in | std::stringstream::out);

        ss << f ;
        return ss.str () ;

}

std::string IntToStr ( float i )
{

        std::stringstream ss (std::stringstream::in | std::stringstream::out);

        ss << i ;
        return ss.str () ;

}

bool isRegolarFile ( std::string nome_file ) {

	struct stat stat_buf;

    if ( stat( nome_file.c_str(), &stat_buf) == 0 )
		return ( stat_buf.st_mode & S_IFREG );
	else
		return false ;


 }

bool FileExists ( const std::string & nome_file )
{

  	std::ifstream ifs( nome_file.c_str(), std::ios_base::in);

	bool isGood = ifs.good() ;

	ifs.close();

	return isGood ;

 }


namespace utils {



	std::string getProcStatus()
	{
	
	

		pid_t pid = getpid();

		std::stringstream ss ;
		ss << "/proc/" << pid << "/status";
		std::ifstream in( ss.str() );

		std::string ret="{";
		while ( in.good() ) {

			std::string line;
			std::getline( in, line );

			std::size_t p = line.find(":");
			if ( p != std::string::npos ) {
				std::string name = trim(line.substr(0, p));
				std::string value = trim(line.substr(p+1));
				ret += "\"" + name + "\":\"" + value + "\",";
			}

		}
		
		// toglie l'ultimo carattere, la virgola
		ret = ret.substr( 0, ret.size()-1 ) + "}";
		
		return ret;


	}

	/* urldecode - v. RFC 1738 per i dettagli */
	std::string urldecode (   const std::string & input)
	{
        std::string str ;
        char c, c1, c2;
        const char *in = input.c_str();
        while ( (c = *in) ) {
                if( c == '%' ) {
                        c1 = *(++in);
                        c2 = *(++in);
//                        if( c1 == EOF || c2 == EOF )  exit(0);
                        c1 = tolower(c1);
                        c2 = tolower(c2);
                        if( ! isxdigit(c1) || ! isxdigit(c2) )  return str;
                        if( c1 <= '9' )
                                c1 = c1 - '0';
                        else
                                c1 = c1 - 'a' + 10;
                        if( c2 <= '9' )
                                c2 = c2 - '0';
                        else
                                c2 = c2 - 'a' + 10;
                        str +=  16 * c1 + c2 ;
                }
                else if( c == '+' )
                        str +=  ' ' ;
                else
                        str +=  c ;
                in++;
        }
        return str;
	}

	bool is_utf8( const char * string )
	{
		if(!string)
			return 0;

		const unsigned char * bytes = (const unsigned char *)string;
		while(*bytes) {
        
			if( (// ASCII
				// use bytes[0] <= 0x7F to allow ASCII control characters
					bytes[0] == 0x09 ||
					bytes[0] == 0x0A ||
					bytes[0] == 0x0D ||
					(0x20 <= bytes[0] && bytes[0] <= 0x7E)
				)
			) {
			
				bytes += 1;
				continue;
        
			}

			if( (// non-overlong 2-byte
					(0xC2 <= bytes[0] && bytes[0] <= 0xDF) &&
					(0x80 <= bytes[1] && bytes[1] <= 0xBF)
				)
			) {
        
				bytes += 2;
				continue;

			}

			if( (// excluding overlongs
					bytes[0] == 0xE0 &&
					(0xA0 <= bytes[1] && bytes[1] <= 0xBF) &&
					(0x80 <= bytes[2] && bytes[2] <= 0xBF)
				) ||
				(// straight 3-byte
					((0xE1 <= bytes[0] && bytes[0] <= 0xEC) ||
						bytes[0] == 0xEE ||
						bytes[0] == 0xEF) &&
					(0x80 <= bytes[1] && bytes[1] <= 0xBF) &&
					(0x80 <= bytes[2] && bytes[2] <= 0xBF)
				) ||
				(// excluding surrogates
					bytes[0] == 0xED &&
						(0x80 <= bytes[1] && bytes[1] <= 0x9F) &&
						(0x80 <= bytes[2] && bytes[2] <= 0xBF)
				)
			) {

				bytes += 3;
				continue;

			}


			if( (// planes 1-3
					bytes[0] == 0xF0 &&
					(0x90 <= bytes[1] && bytes[1] <= 0xBF) &&
					(0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
					(0x80 <= bytes[3] && bytes[3] <= 0xBF)
				) ||
				(// planes 4-15
					(0xF1 <= bytes[0] && bytes[0] <= 0xF3) &&
					(0x80 <= bytes[1] && bytes[1] <= 0xBF) &&
					(0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
					(0x80 <= bytes[3] && bytes[3] <= 0xBF)
				) ||
				(// plane 16
					bytes[0] == 0xF4 &&
					(0x80 <= bytes[1] && bytes[1] <= 0x8F) &&
					(0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
					(0x80 <= bytes[3] && bytes[3] <= 0xBF)
				)
			) {

				bytes += 4;
				continue;

			}

			return false;
		}

		return true;

	}

	std::string file_get_contents ( const std::string & file_name ) 
	{
	
		std::string ret;
		std::ifstream ifs ( file_name.c_str(), std::ifstream::in );

		if ( ifs.good() ) {

			// get length of file:
			ifs.seekg (0, std::ios::end) ;
			size_t length = ifs.tellg () ;
			ifs.seekg (0, std::ios::beg) ;

			// allocate memory:
			char * buffer = new char [ length + 1 ];

			// read data as a block:
			ifs.read (buffer, length);
			buffer [ length ] = '\0';

			ret = buffer ;
		
			ifs.close();

			delete [] buffer;

		}
		else {
		
			int ret = access( file_name.c_str(), 0 );
			if ( ret == -1 )
				throw ( std::string("Il file " + file_name + " non esiste") );

			ret = access( file_name.c_str(), 4 );
			if ( ret == -1 )
				throw ( std::string("Non si hanno i permessi di lettura del file " + file_name) );
	
		}
	
		return ret;
	
	}

	void file_set_contents ( const std::string & file_name, const std::string & content  ) 
	{
	

		std::fstream ofs ( file_name.c_str(),  std::fstream::in | std::fstream::out | std::fstream::app );
		if ( ofs.good() ) {

			ofs << content << std::endl;

		}
		else {
		
			int ret = access( file_name.c_str(), 0 );
			if ( ret == -1 )
				throw ( std::string("Il file " + file_name + " non esiste") );

			ret = access( file_name.c_str(), 4 );
			if ( ret == -1 )
				throw ( std::string("Non si hanno i permessi di lettura del file " + file_name) );
	
		}
		
		ofs.close();
	

	}

	size_t split ( const std::string& input, const std::string& delimiter, std::vector<std::string> &v )
	{

		size_t iniz = 0;
		size_t l_delim = delimiter.size();

		size_t n = input.find(delimiter);
		while (n != std::string::npos) {

			v.push_back(input.substr( iniz, n - iniz ) );
			iniz = n + l_delim;
			n = input.find(delimiter, iniz);
	
		}

		v.push_back(input.substr(iniz, input.size()));

		return v.size();

	}

	std::string trim ( const std::string &str )
	{

		size_t i = 0;
		size_t j = str.size() - 1;
		for ( ; i <=  j; i++ ) if ( !isspace( str[i] ) ) break;
		for ( ; j >= i; j-- ) if ( !isspace( str[j] ) ) break;

		return str.substr ( i, j + 1 - i );

	}

	std::string getWord ( const std::string & str, size_t & i )  
	{
		// estrae la prima parola e avanza i alla posizione successiva alla fino della parola
		// se tra apici considera ciò che c'è tra questi

		// elimina gli spazi 
		while (  i < str.size() && isblank( str.at(i) ) ) ++i;
	
		size_t start = i;
		bool tra_apici = false;
		while ( i < str.size() ) {

			if ( str.at( start ) == '"' ) {
				start++;
				tra_apici = true;
			}
			else if ( !tra_apici && isblank( str.at(i) ) ) {
				return str.substr ( start, i - start );
			}
			else if ( tra_apici && str.at( i ) == '"' ) {
				size_t end = i ;
				i++;
				return str.substr ( start, end - start );
			}
			else if ( str.at( i ) == '\'' ) {
				size_t end = ++i ;
				return str.substr ( start, end - start );
			}

			i++;

		}
	
	
		size_t end = i ;
		return str.substr ( start, end - start );

	}


	bool isNumber ( const std::string & number ) {
		return isNumber ( number.c_str() );
	}

	bool isNumber ( const char * number ) {

		/*
		* formato numero
		* 250.5
		* 2.5E2
		* 2.5E+2
		*
		* 0.025
		* 2.5E-2
		* */
		const char *p = number;
		while ( *p && isspace( *p ) ) p++ ;
		if ( !*p ) return false;

		if ( *p == '-' || *p == '+' || isdigit( *p ) ) {

			if ( !isdigit( *p ) && strlen( p ) == 1 ) return false;
			else if ( !isdigit( *p ) ) p++;

			size_t dot_count=0;
			size_t exp_count=0;
			while ( *p ) {
				if ( *p == '.' ) dot_count++;
				else if ( *p == 'E' ) {
					exp_count++;
					// dopo l'esponente può seguire il segno + o -
					if ( *p && (*p=='-' || *p=='+') )
						p++;
					// quindi solo numeri non più punto
					if ( dot_count == 0 ) dot_count = 1;
				}
				else if ( !isdigit( *p ) ) return false;
				p++;
			}

			// ci può essere un solo punto e un solo esponente
			if ( dot_count >1 || exp_count > 1 ) return false;
			else return true;

		}

		return false;

	}
 
	std::string & str_replace( const std::string & search, const std::string &replace, std::string & str )
	{


		std::size_t p = str.find(search);
		while ( p != std::string::npos ) {
			str.replace ( p, search.size(), replace );
            p = str.find( search, p );
		}

		return str;

	}
	std::string & toUpper ( std::string & s )
	{
		std::transform(s.begin(), s.end(), s.begin(), (int(*)(int)) toupper);
		return s;
	}


	std::string getValue ( const std::string &from_text, const std::string &name, const std::string &separatore )
	{

		size_t found = from_text.find ( name + "=" );
		if ( found != std::string::npos )	{
			size_t len = name.length() + 1 ;
			size_t start = found + len;
			std::string remaining = from_text.substr ( found + len );
			size_t end = remaining.find ( separatore ) ;
			if ( end == std::string::npos ) end = remaining.length();
			return utils::trim(from_text.substr ( start, end ));
		}
		else
			return "";

	}


	void replace ( const std::string & da_str, const std::string & a_str, std::string & nel_testo )
	{

		std::string::size_type found = nel_testo.find ( da_str );
		while ( found != std::string::npos ) {
			nel_testo.replace (found, da_str.size(), a_str );
			found = nel_testo.find ( da_str, found + 1 );
		}

	}
  
  
  json listFilesInDirectory(const std::string& path) {
      json result;
      result["directory"] = path;
      result["files"] = json::array();

      try {
          if (fs::exists(path) && fs::is_directory(path)) {
              for (const auto& entry : fs::directory_iterator(path)) {
                  if (fs::is_regular_file(entry.path())) {
                      json fileInfo;
                      fileInfo["name"] = entry.path().filename().string();
                      fileInfo["path"] = entry.path().string();
                      fileInfo["size"] = fs::file_size(entry.path());
                      result["files"].push_back(fileInfo);
                  }
              }
          } else {
              result["error"] = "Directory non valida";
          }
      } catch (const std::exception& e) {
          result["error"] = e.what();
      }

      return result;
  }

// --- Funzione Base64 ---
static const std::string base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

std::string base64_encode(const std::string &in) {
    std::string out;
    int val=0, valb=-6;
    for (unsigned char c : in) {
        val = (val<<8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(base64_chars[(val>>valb)&0x3F]);
            valb -= 6;
        }
    }
    if (valb>-6) out.push_back(base64_chars[((val<<8)>>(valb+8))&0x3F]);
    while (out.size()%4) out.push_back('=');
    return out;
}

// --- Funzione compressione con zlib ---
std::string compressToWebSocket(const std::string& str, int level ) {
    z_stream zs;
    memset(&zs, 0, sizeof(zs));

    if (deflateInit(&zs, level) != Z_OK) {
        throw std::runtime_error("deflateInit failed");
    }

    zs.next_in = (Bytef*)str.data();
    zs.avail_in = str.size();

    int ret;
    char outbuffer[32768];
    std::string compressed;

    do {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);

        ret = deflate(&zs, Z_FINISH);

        if (compressed.size() < zs.total_out) {
            compressed.append(outbuffer, zs.total_out - compressed.size());
        }
    } while (ret == Z_OK);

    deflateEnd(&zs);

    if (ret != Z_STREAM_END) {
        throw std::runtime_error("Exception during zlib compression");
    }

    // Codifica Base64 pronto per WebSocket (testo)
    return base64_encode(compressed);
}



}	// namespace utils

std::string Sostituisci ( std::string da_str, std::string a_str, std::string nel_testo )
{

	std::string::size_type found = nel_testo.find ( da_str );
	while ( found != std::string::npos ) {
		nel_testo.replace (found, da_str.size(), a_str );
		found = nel_testo.find ( da_str, found + 1 );
	}

	return ( nel_testo );

}

// calcola il codice Hash
std::string strHash ( std::string str )
{

	const char *s = str.c_str();
	const unsigned int HASHSIZE = 1000000 ;

	unsigned ashvalue = 0;
	for (; *s != '\0'; s++)
    	ashvalue = *s + 31 * ashvalue;

	ashvalue %= HASHSIZE;

	std::ostringstream oss ;

	oss << ashvalue ;

	return oss.str() ;

}


#if defined(__cplusplus) && __cplusplus
 extern "C" {
#endif


char * strupr(char * str )
{
      char *s;

      if ( str )
      {
            for (s = str; *s; ++s)
                  *s = toupper(*s);
      }
      return str;
}

char * strlwr(char *str)
{
      char *s;

      if ( str )
      {
            for (s = str; *s; ++s)
                  *s = tolower(*s);
      }
      return str;
}

#if defined(__cplusplus) && __cplusplus
 }
#endif



int GetStrTraChar (const char *str, char ch) ;

// ritorna la posizione dopo la parentesi tonda di chiusura
// -1 se manca una parentesi chiusa
// -2 se manca un parentesi aperta
int GetStrInBraket (const char *str)
{

	int i = 0 ;
	int braket = 0;

   // parte dal successivo
   // se la stringa non é finita
   // o il carattere ) e braket=0 (tante parentesi chiuse quante aperte)
	while ((*str != 0) && (*str != '\n') && !(!braket && (*str == ')'))) {
		if (*str == '(')
			braket++;
		if (*str == ')')
			braket--;
		i++;
      str++;
	}

	if (braket > 0)
		return -1 ;	// ("Era attesa una parentesi chiusa");
	else if (braket < 0)
		return -2 ; // ("Era attesa una parentesi aperta");

   return ++i;

}

int GetStrTraDoppiApicini(const char *str)
{
	return GetStrTraChar (str, '"') ;
}

int GetStrTraApicini(const char *str)
{
	return GetStrTraChar (str, '\'') ;
}

int GetStrTraChar (const char *str, char ch)
{

	int i = 0;

   // parte dal successivo
   // se la stringa non é finita
   // o il carattere apicino
	while ((*str != '\0') && (*str != '\n') && (*str != ch))
		i++ ;

	if (*str != ch)
		return -1 ; // "manca carattere di chiusura");

// ritorna la posizione dopo il caraterre ch di chiusura
	return ++i;
}

char * LTrim (char * arg)
{
// elimina gli spazi bianchi a sinistra della stringa arg
// e ritorna il nuovo puntatore
	while ( isspace(*arg)  && *arg != '\0')
		arg++;
	return arg;
}

/*
// attenzione modifica arg, mette carattere di fine stringa
char * RTrim (char * arg)
{
// elimina gli spazi bianchi e caratteri LF a destra della stringa arg
// e ritorna il nuovo puntatore
	int len = strlen(arg) ;

	for ( unsigned int i = len - 1; isspace(arg[i]) && i >= 0; --i )
		arg[i] = '\0';


	return arg;
}
*/

// attenzione modifica arg, mette carattere di fine stringa
char *RTrim(char *arg) {
// elimina gli spazi bianchi e caratteri LF a destra della stringa arg
// e ritorna il nuovo puntatore
    size_t len = strlen(arg);
    while (len > 0 && isspace((unsigned char)arg[len - 1])) {
        arg[--len] = '\0';
    }
    return arg;
}

char * DelBlk(char * arg)
{
	return RTrim (LTrim(arg)) ;
}

char * lLTrim(char * arg)
{
// elimina gli spazi bianchi a sinistra della stringa arg
// e ritorna il nuovo puntatore
	while (*arg == ' ' && *arg != '\0')
		arg++;
	return arg;
}

char * lRTrim(char * arg)
{
// elimina gli spazi bianchi a destra della stringa arg
// e ritorna il nuovo puntatore

	int len = strlen(arg);

	while (arg[len-1] == ' ' && len >= 0)
		len--;

	arg[len] = '\0';

	return arg;

}

char * lDelBlk(char * arg)
{
	return lRTrim(lLTrim (arg)) ;
}

std::string toLower ( std::string str ) {

	std::string::iterator it;
	for ( it=str.begin() ; it < str.end(); it++ )
		*it = tolower( *it );
	return str;

}

std::string toUpper ( std::string str ) {

	std::string::iterator it;
	for ( it=str.begin() ; it < str.end(); it++ )
		*it = toupper( *it );
	return str;

}

std::string Trim ( const std::string &str )
{

	if ( str == "" ) return "";

	size_t start = 0, end = str.length () - 1 ;
	for ( ; start <= end; ++start )
		if ( !( isspace ( str[start] ) || iscntrl( str[start] ) ) ) break;

	for ( ; end >= start; --end )
		if ( !( isspace ( str[end] ) || iscntrl( str[end] ) ) )  break;

	return str.substr ( start, end - start + 1 );

}

int lstrncmp(const char * s1, const char * s2, int n)
{
	int cmp;
	for (int i = 0; i < n; i++, s1++, s2++) {
		cmp = (*s1 - *s2) ;
		if (cmp)
			break;
	}
	return cmp;
}

char * lstrncpy( char * s1, char * s2, int n )
{
        char * olds1 = s1;
	for (int i = 0; i < n; i++, s1++, s2++)
		*s1 = *s2;
	*s1 = '\0';
        return olds1;
}

char * lstrncat( char * s1, char * s2, int n )
{
	char * olds1 = s1;
	while (*s1++)
		;
        --s1;
	for (int i = 0; i < n; i++)
        	*s1++ = *s2++;
        *s1 = '\0';
	return olds1;
}

char * lStrChr(char * sel, char * arg, char chr)
{
/* estrae da arg la stringa fino alla prima occorrenza di chr
   o fino a fine stringa e la copia in sel
   arg punta al carattere successivo a chr o al carattere di fine stringa
   */

	if (*arg == '\0') return NULL;
	int i = 0;

	while (*arg != chr) {
		if (*arg == '\0' || *arg == '\n') {
			sel[i] = '\0';
			return arg;
	        }
	        sel[i++] = *arg++;
        }
	sel[i] = '\0';
	return ++arg;
}

int strncmp_no_case(const char * s1, const char * s2, int n)
{
	int cmp;
	for (int i = 0; i < n; i++, s1++, s2++) {
		cmp = ( toupper((unsigned char)*s1) - toupper((unsigned char)*s2) ) ;
		if (cmp)
			break;
	}
	return cmp;
}

bool fileExists ( const std::string & strFilename )
{

    struct stat stFileInfo;
    int intStat;

    // Attempt to get the file attributes
    intStat = stat(strFilename.c_str(),&stFileInfo);

    if ( intStat == 0 ) {
        // We were able to get the file attributes
        // so the file obviously exists.
        return true;
    } else {
        /*
        We were not able to get the file attributes.
        This may mean that we don't have permission to
        access the folder which contains this file. If you
        need to do that level of checking, lookup the
        return values of stat which will give you
        more details on why stat failed.
        */
        return false;
  }

}

// =======================================
// C++Wrapper around popen
//   that recognaizes invalid command_line
// =======================================

std::string p_open::out ()
{
	std::string str = "";
	for ( unsigned int i = 0; i < out_result_o.size(); i++)
		str += out_result_o[i] + "\n";
	return str;
}

p_open::p_open ( )
{

}

p_open::p_open ( std::string &command_line_i ) :
    p_open ( command_line_i.c_str() )
{
}

p_open::p_open ( const char * command_line_i )
{

	if ( ! _popen ( std::string (command_line_i) ) ) {
       	std::string exc = "Errore di esecuzione : popen " ;
		for ( unsigned int i = 0; i < err_result_o.size(); i++)
			exc += err_result_o[i] + "\n";
		throw exc ;
	}

}

bool p_open::_popen ( const std::string&   command_line_i )
{

	bool            ret_boolValue = true;

	FILE*           fp_outfile;
	FILE*           fp_errfile;

	const int       SIZEBUF = 1234;
	char            buf [SIZEBUF];

	const int       FD_STDERR = 2;
	int             fd_errfile;
	int             fd_new_errfile;
	int             fd_save_error;

	std::string          cur_string;


// ################################################
// ######## Redirection : stderr to errfile #######
// ################################################
// ======================
	if ((fp_errfile = tmpfile ()) == NULL)  {
		FATAL_MSG ("Cannot execute tmpfile ()");
	}

	fd_errfile = fileno (fp_errfile);

	// ======================

	// ======================
	// redirect error to file
	if ((fd_save_error = dup (FD_STDERR)) == -1)  {
		FATAL_MSG ("Cannot execute dup");
	}

	// Now fd_save_error and 2 are descriptors of the screen
	// ======================


	// ======================
	// Close screen
	if (close (FD_STDERR) == -1)  {
		FATAL_MSG ("Cannot close descriptor");
	}

	// Now descriptor 2 (FD_STDERR) is vacant
	// ======================

	// ======================
	if ((fd_new_errfile = dup (fd_errfile)) == -1) {
		FATAL_MSG ("Cannot execute dup");
	}

	// Now fd_errfile and 2 are descriptors of the errfile
	// ======================


	// ##########################
	/* This is in comment
	  // ======================
	if (close (fd_errfile) == -1)  {
		FATAL_MSG ("Cannot close descriptor");
	}

	// Now only descriptor 2 (FD_STDERR) is descriptor of the errfile
	// ======================
	*/
	// ##########################


	// ################################################
	// ################# popen ########################
	// ################################################
	if ((fp_outfile = popen(command_line_i.c_str (), "r")) == NULL)  {
		FATAL_MSG ("Files or processes cannot be created");
		ret_boolValue = false;
	}


	// ################################################
	// ### Get out stream ############################
	// ################################################
	// ================================
	out_result_o = std::vector<std::string> ();
	// ================================
	if (ret_boolValue) {
		while (fgets(buf, sizeof (buf), fp_outfile)) {
			cur_string = buf;
			if (cur_string [cur_string.size () - 1] != '\n')  {
				ERROR_MSG ("SIZEBUF too small (" << SIZEBUF << ") or missing '\\n'");
				out_result_o.push_back ("!!! Cannot get this out_line !!!");
				ret_boolValue = false;
				break;
			}

			// ------------------------------------
			assert ( cur_string [cur_string.size () - 1] == '\n' );
			out_result_o.push_back (cur_string.substr (0, cur_string.size () - 1));
			// ------------------------------------
		} // while (fgets(buf, sizeof (buf), fp_outfile))

		// ================================
		if (pclose(fp_outfile) == -1) {
			FATAL_MSG ("Cannot execute pclose");
			ret_boolValue = false;
		}
	} // if (ret_boolValue)

	// ======================
	// Close screen
	if (close (FD_STDERR) == -1) {
		FATAL_MSG ("Cannot close descriptor");
	}
	// Now descriptor 2 (FD_STDERR) is vacant
	// ======================

	// ======================
	if ((fd_new_errfile = dup (fd_errfile)) == -1) {
		FATAL_MSG ("Cannot execute dup");
	}
	// Now fd_errfile and 2 are descriptors of the errfile
	// ======================



	// ################################################
	// ### Restore : error stream to screen ###########
	// ################################################
	// ======================
	// redirect (restore) error to screen
	// Close screen
	if (close (FD_STDERR) == -1) {
		FATAL_MSG ("Cannot close descriptor");
	}

	// Now descriptor 2 (FD_STDERR) is vacant
	// ======================

	// ======================
	if (dup (fd_save_error) == -1) {
		FATAL_MSG ("Cannot execute dup");
	}

	// Now fd_save_error and 2 are descriptors of the screen
	// ======================

	// ======================
	if (close (fd_save_error) == -1) {
		FATAL_MSG ("Cannot close descriptor");
	}

	// Now only descriptor 2 (FD_STDERR) is descriptor of the screen

	// ======================


	// ################################################
	// ### Get error stream ###########################
	// ################################################
	// ================================
	err_result_o = std::vector<std::string> ();
	// ================================
	rewind (fp_errfile);
	while (fgets(buf, sizeof (buf), fp_errfile)) {
		cur_string = buf;
		if (cur_string [cur_string.size () - 1] != '\n') {
			ERROR_MSG ("SIZEBUF too small (" << SIZEBUF << ") or missing '\\n'");
			err_result_o.push_back ("!!! Cannot get this error-line !!!");
			ret_boolValue = false;
			break;
		}

		 // ------------------------------------
		assert (cur_string [cur_string.size () - 1] == '\n');
		err_result_o.push_back (cur_string.substr (0, cur_string.size() - 1));
		// ------------------------------------
	} // while (fgets(buf, sizeof (buf), fp_errfile))

	if (!err_result_o.empty ()) {
		ret_boolValue = false;
	}

	// ===================
	return ret_boolValue;
	// ===================

} // bool popen_cplusplus_wrapper (...)

void log_file ( const std::string & msg )
{

	const char * DTTMFMT = "%d-%m-%Y %H:%M:%S ";
	const int DTTMSZ = 21;

    char buff[DTTMSZ];

	time_t t = time (0);
	strftime (buff, DTTMSZ, DTTMFMT, localtime (&t));
	std::string ti = buff;


	std::ofstream myfile;

	myfile.open ( "/tmp/appXml.log", std::ios::out | std::ios::app );
	myfile << ti << " " << msg << std::endl;
	myfile.close();

}

void log_file ( float msg )
{

	std::ostringstream log ( "" );
	log <<  msg ;

	log_file ( log.str() );


}
