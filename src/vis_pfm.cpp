#include "image.h"
#include "imio.h"

#include <climits>
#include <string>
#include <fstream>
#include "cam_util.h"


int main(int argc, char* argv[])
{
	if (argc < 2)
		return 1;
	auto in = img::imread<float,1>(argv[1]);
	auto disc = img::Img<uint16_t>(in.width, in.height);
	auto hist = img::Image<uint8_t, 3>(in.width, in.height);
	if (argc >= 3) {
		auto conf = img::imread<float, 1>(argv[2]);
		for (int i = 0; i < in.width*in.height; i++)
			in.ptr[i] = conf.ptr[i] > 0.5 ? in.ptr[i] : 0.0f;
	}
	for (int i = 0; i < in.width*in.height; i++)
		disc.ptr[i] = (uint16_t)( in.ptr[i] * 8 + 0.5f);

	util::ConvertDepthToRGBUsingHistogram(hist.ptr, disc.ptr, in.width, in.height, 0, 0.625f);

	//std::string newfile(argv[1]);
	//newfile[newfile.size() - 1] = 'g';
	//newfile[newfile.size() - 2] = 'n';
	//newfile[newfile.size() - 3] = 'p';
	//img::imwrite(newfile.c_str(), hist);
	img::imwrite("out.png", hist);
	return 0;
}
