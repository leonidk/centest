#include "imio.h"
#include "cMatch.h"
#include "bmMatch.h"
#include "image_filter.h"

#include <string>

int main(int argc, char* argv[])
{
    if(argc < 3)
        return 1;

    std::string leftFile(argv[1]);
    std::string rightFile(argv[2]);

    auto left = img::imread<uint8_t,3>(argv[1]);
    auto right = img::imread<uint8_t,3>(argv[2]);
   
    auto left_g = img::Rgb2grey(left);
    auto right_g = img::Rgb2grey(right);
	stereo::BMatch cm(left.width, left.height, 40, 4);
	auto disp = cm.match(left_g, right_g);

	auto ot = img::Image<uint8_t, 1>(disp.width, disp.height);
    auto dptr = disp.data.get();
    auto optr = ot.data.get();
	memset(optr, 0, disp.width*disp.height);

	float mse = 0;
	auto sqr = [](float a) {  return a*a; };
    for(int i=0; i < ot.width*ot.height; i++)
    {
        optr[i] = (uint8_t)dptr[i];
    }
    img::imwrite("disp-out.png",ot);
	return 0;
}
