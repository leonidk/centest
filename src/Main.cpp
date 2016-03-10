#include "imio.h"
#include "cMatch.h"
#include "bmMatch.h"
#include "image_filter.h"
int main(int argc, char* argv[])
{
    auto left = img::imread<uint8_t,3>("poster/u-1.png");
    auto right = img::imread<uint8_t,3>("poster/u-ref.png");
	//auto gt = img::imread<uint8_t,1>("test_data/disp0-1.png");
   
    auto left_g = img::Rgb2grey(left);
    auto right_g = img::Rgb2grey(right);
	stereo::BMatch cm(left.width, left.height, 40, 4);
	auto disp = cm.match(left_g, right_g);

	auto ot = img::Image<uint8_t, 1>(disp.width, disp.height);
    auto dptr = disp.data.get();
    auto optr = ot.data.get();
	//auto gptr = gt.data.get();
	memset(optr, 0, disp.width*disp.height);

	float mse = 0;
	auto sqr = [](float a) {  return a*a; };
    for(int i=0; i < ot.width*ot.height; i++)
    {
        optr[i] = (uint8_t)dptr[i];
		//mse += gptr[i] && dptr[i] && gptr[i] != 255 ? sqr(gptr[i] - dptr[i]) : 0;
    }
	printf("%f\n", sqrt(mse / (ot.width*ot.height)));
    img::imwrite("poster/u-out.png",ot);
	return 0;
}
