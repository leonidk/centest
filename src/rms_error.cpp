#include <memory>
#include "bmMatch.h"
#include "cMatch.h"
#include "imio.h"
#include "r200Match.h"
#include "sgbmMatch.h"

#include "image_filter.h"

#include <string>
#include <fstream>
#include <chrono>
#include <cmath>
#include <iostream>
#define JSON_H_IMPLEMENTATION
#include "json.h"

int main(int argc, char* argv[])
{
	json::value doc;
    if (argc < 2)
        return 1;
    if (auto in = std::ifstream(argv[1]))
    {
        std::string str((std::istreambuf_iterator<char>(in)),
            std::istreambuf_iterator<char>());
        doc = json::parse(str);
    }
    else {
        std::vector<std::string> args(argv + 1, argv + argc);
        auto str = accumulate(begin(args), end(args), std::string());
        doc = json::parse(str);
	}
	std::string leftFile = doc["left_rgb"].string();
    std::string rightFile = doc["right_rgb"].string();

    auto left = img::imread<uint16_t, 3>(leftFile.c_str());
    auto right = img::imread<uint16_t, 3>(rightFile.c_str());
    
    auto left_g = img::Rgb2grey(left);
    auto right_g = img::Rgb2grey(right);
    
	int bitshift = (int)log2(doc["minint"].number<int>()+1);

	for (int i = 0; i < left.width*left.height; i++) {
		left_g(i) >>= bitshift;
		right_g(i) >>= bitshift;
	}

	auto disp = img::imread<float,1>(doc["input_disp"].string().c_str());
	auto conf = img::imread<float,1>(doc["input_conf"].string().c_str());

	auto gt_disp = img::imread<float,1>(doc["gt_disp"].string().c_str());
	auto gt_mask = img::imread<float,1>(doc["gt_conf"].string().c_str());
    return 0;
}
