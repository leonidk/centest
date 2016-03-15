#include "../src/imio.h"
#include <opencv\cv.h>

int main(int argc, char* argv[])
{
	if (argc < 4) {
		printf("Usage: %s <fov> <input> <output>\n", argv[0]);
		return 1;
	}

	int hfov = atoi(argv[1]);
	auto input_fn = argv[2];
	auto output_fn = argv[3];

	auto img = img::imread<uint8_t, 3>(input_fn);
	auto img_out = img::Image<uint8_t, 3>(img.width, img.height);
	auto ptr = img.data.get();
	auto ptr2 = img_out.data.get();

	cv::Mat remap1(img.height, img.width, CV_32FC2),cv_out;
	cv::Mat cv_in(img.height, img.width,CV_8UC3);
	memcpy(cv_in.data, img.data.get(), 3 * img.width*img.height);

	for (int y = 0; y < img_out.height; y++) {
		for (int x = 0; x < img_out.width; x++) {
			for (int c = 0; c < 3; c++){
				auto xc = static_cast<float>(x);
				auto yc = static_cast<float>(y);
				float hw = img.width / 2.0f;
				float hh = img.height / 2.0f;
				auto xx = (xc - hw);
				auto yy = (yc - hh);
				//0.05
				auto dist = sqrt(xx*xx + yy*yy);
				auto maxDist = sqrt(hw*hw + hh*hh);
				auto weight = dist / maxDist;

				auto mapped_match = xx / hw;
				auto orig_match = atan(mapped_match*tan(1.0));
				auto match = orig_match*hw + hw;

				auto mapped_matchy = yy / hh;
				auto orig_matchy = atan(mapped_matchy*tan(1.0));
				auto matchy = orig_matchy*hh + hh;

				remap1.at<cv::Vec2f>(y, x) = cv::Vec2f(match, matchy);
			}
		}
	}
	cv::remap(cv_in, cv_out, remap1, cv::Mat(), cv::INTER_LINEAR);
	memcpy(img_out.data.get(), cv_out.data, 3 * img.width*img.height);

	img::imwrite(output_fn, img_out);
	return 0;
}