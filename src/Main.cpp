#include "imio.h"
#include "cMatch.h"
#include "image_filter.h"
int main(int argc, char* argv[])
{
    auto left = img::imread<uint8_t,3>("highres/im0.png");
    auto right = img::imread<uint8_t,3>("highres/im1.png");
	auto gt = img::imread<uint8_t,1>("highres/im1.png");
   
    auto left_g = img::Rgb2grey(left);
    auto right_g = img::Rgb2grey(right);
	stereo::CensusMatch cm(left.width, left.height, 270, 1);
	auto disp = cm.match(left_g, right_g);

    auto ot = img::Image<uint8_t,1>(gt.width,gt.height);
    auto dptr = disp.data.get();
    auto optr = ot.data.get();
	auto gptr = gt.data.get();
	memset(optr, 0, gt.width*gt.height);

	float mse = 0;
	auto sqr = [](float a) {  return a*a; };
    for(int i=0; i < ot.width*ot.height; i++)
    {
        optr[i] = dptr[i]/1;
		mse += gptr[i] && dptr[i] ? sqr(gptr[i] - dptr[i]) : 0;
    }
	printf("%f\n", sqrt(mse / (ot.width*ot.height)));
    img::imwrite("out_disp.png",ot);
	return 0;
}
