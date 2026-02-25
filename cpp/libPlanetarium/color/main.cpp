#include <color.hpp>

int main (int /* argc */, char ** /* argv[] */ )
{
	
	RGB rgb( 237.0/255.0, 11.0/255.0, 11.0/255.0 );
	std::cout << rgb.R << " " << rgb.G << " " << rgb.B << std::endl;
	std::string str = rgb;
	
	HSV hsv=rgb;
	
	std::cout << hsv.H << " " << hsv.S << " " << hsv.V << std::endl;
	hsv.H=50;
	
	rgb=hsv;
	std::cout << rgb.R << " " << rgb.G << " " << rgb.B << std::endl;
	
	return (0);

}
