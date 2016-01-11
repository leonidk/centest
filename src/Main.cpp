#include "imio.h"
#include "cMatch.h"
#include "image_filter.h"
int main(int argc, char* argv[])
{
    auto left = img::imread<uint8_t,3>("cones/im2.png");
    auto right = img::imread<uint8_t,3>("cones/im6.png");
	auto gt = img::imread<uint8_t,1>("cones/disp2.png");
   
    auto left_g = img::Rgb2grey(left);
    auto right_g = img::Rgb2grey(right);
    stereo::CensusMatch cm(left.width, left.height, 64);
	
    auto disp = cm.match(left_g, right_g);
    
	return 0;
}
