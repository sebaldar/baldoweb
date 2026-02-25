#include <fstream>
#include <sstream>
#include <iostream>
#include <json.hpp>

int main()
{
	JSON json;
	json.loadFromFile("test1.json");
	

	std::vector< std::string > array = json.array("array");
	std::cout << array[2] << std::endl;


}
