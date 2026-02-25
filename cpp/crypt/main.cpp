#include <iostream>
#include "sha1.hpp"
#include "base64.hpp"

using namespace std;
 
int main(int argc, char *argv[])
{

	std::string key ="x3JJHMbDL1EzLkh9GBhXDw==";
	std::string check = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	std::string str = key + check;
    
    std::string dec =  sha1( str );
    std::cout << "sha1 : " << dec << endl;
    std::cout << "base64 : " << base64_encode( dec ) << endl;


}
