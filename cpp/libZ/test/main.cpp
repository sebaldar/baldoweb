// Copyright 2007 Timo Bingmann <tb@panthema.net>
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <cstring>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "zlib.hpp"

/** Small dumb tool (de)compressing cin to cout. It holds all input in memory,
  * so don't use it for huge files. */
int main(int argc, char* argv[])
{

	Zlib zlib;

    std::string allinput;

    while (std::cin.good())     // read all input from cin
    {
        char inbuffer[32768];
        std::cin.read(inbuffer, sizeof(inbuffer));
        allinput.append(inbuffer, std::cin.gcount());
    }

    if (argc >= 2 && strcmp(argv[1], "-d") == 0)
    {

        std::string cstr = zlib.decompress_string( allinput );

        std::cerr << "Inflated data: "
                  << allinput.size() << " -> " << cstr.size()
                  << " (" << std::setprecision(1) << std::fixed
                  << ( ((float)cstr.size() / (float)allinput.size() - 1.0) * 100.0 )
                  << "% increase).\n";

        std::cout << cstr;
    }
    else
    {
        std::string cstr = zlib.compress_string( allinput );

        std::cerr << "Deflated data: "
                  << allinput.size() << " -> " << cstr.size()
                  << " (" << std::setprecision(1) << std::fixed
                  << ( (1.0 - (float)cstr.size() / (float)allinput.size()) * 100.0)
                  << "% saved).\n";

        std::cout << cstr;
    }
}
