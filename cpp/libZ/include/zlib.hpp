#ifndef _zlib_
#define _zlib_

#include <iostream>

#include <zlib.h>

// Found this one here: http://panthema.net/2007/0328-ZLibString.html, author is Timo Bingmann
// edited version
/** Compress a STL string using zlib with given compression level and return
  * the binary data. */

class Zlib {

// Found these here http://mail-archives.apache.org/mod_mbox/trafficserver-dev/201110.mbox/%3CCACJPjhYf=+br1W39vyazP=ix
//eQZ-4Gh9-U6TtiEdReG3S4ZZng@mail.gmail.com%3E
	const short MOD_GZIP_ZLIB_WINDOWSIZE = 15;
	const short MOD_GZIP_ZLIB_CFACTOR =   9;
	const short MOD_GZIP_ZLIB_BSIZE =  8096;
	
	std::string outstring;
	
public :

	Zlib();
	virtual ~Zlib ();
	
	/** Compress a STL string using zlib with given compression level and return
	  * the binary data. */
	std::string & zip (const std::string& str,
                            int compressionlevel = Z_BEST_COMPRESSION);

	/** Decompress an STL string using zlib and return the original data. */
	std::string & unzip (const std::string& str);

	std::string & gzip(const std::string& str,
                             int compressionlevel = Z_BEST_COMPRESSION);

	std::string & gunzip(const std::string& str) ;

};



#endif	// if _lib_
