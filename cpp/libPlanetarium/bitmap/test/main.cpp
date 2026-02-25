#include <bitmap_image.hpp>

int main(int /* argc */, char ** /* argv */)
{

	bitmap_image bmp ( 10, 10 );
	bmp.clear();

	rgb_t c ( 255, 0, 0 );
	
	bmp.set_pixel( 1, 1, c);
	bmp.set_pixel( 2, 1, c);

	bmp.save_image("test.bmp");

	return ( 0 );

}
