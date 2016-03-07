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
    stereo::CensusMatch cm(left.width, left.height, 64, 16);	
    auto disp = cm.match(left_g, right_g);

    auto ot = img::Image<uint8_t,1>(gt.width,gt.height);
    auto dptr = disp.data.get();
    auto optr = ot.data.get();
    for(int i=0; i < ot.width*ot.height; i++)
    {
        optr[i] = dptr[i]/4;
    }
    img::imwrite("out_disp.png",ot);
	return 0;
}
