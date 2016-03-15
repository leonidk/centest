#include "imio.h"
#include "cMatch.h"
#include "bmMatch.h"
#include "image_filter.h"
int main(int argc, char* argv[])
{
    auto left = img::imread<uint8_t,3>(R"(rectification/left-both-x2.png)");
	auto right = img::imread<uint8_t, 3>(R"(rectification/right-both-x2.png)");
	//auto gt = img::imread<uint8_t,1>("test_data/disp0-1.png");
   
    auto left_g = img::Rgb2grey(left);
    auto right_g = img::Rgb2grey(right);
	stereo::BMatch cm(left.width, left.height, 127, 2);
	auto disp = cm.match(left_g, right_g);

	auto ot = img::Image<uint8_t, 1>(disp.width, disp.height);
    auto dptr = disp.data.get();
    auto optr = ot.data.get();
	//auto gptr = gt.data.get();
	memset(optr, 0, disp.width*disp.height);
#ifdef REMAP_X
	auto hw = ot.width / 2.0f;
	for (int y = 0; y < ot.height; y++) {
		for (int x = 0; x < ot.width; x++){
			float v = *ptr;
			float xr = x - v / 2;
			auto mapped_match = (xr - hw) / hw;
			auto orig_match = asin(mapped_match*sin(1.0));
			auto match = orig_match*hw + hw;

			auto mapped = (x - hw) / hw;
			auto orig = asin(mapped*sin(1.0));
			auto coord = orig*hw + hw;
			*ptr = 2 * std::round(coord - match);
			printf("%lf %lf\n", coord, match);
			ptr++;
		}
	}
#endif
	float mse = 0;
	auto sqr = [](float a) {  return a*a; };
    for(int i=0; i < ot.width*ot.height; i++)
    {
        optr[i] = dptr[i] ? (uint8_t)(dptr[i]*0.8 + 31) : 0;
		mse += dptr[i] ? 1 : 0;
		//mse += gptr[i] && dptr[i] && gptr[i] != 255 ? sqr(gptr[i] - dptr[i]) : 0;
    }
	printf("%f\n", (mse / (ot.width*ot.height)));
	img::imwrite(R"(rectification/disp-both-x2.png)", ot);
	return 0;
}
