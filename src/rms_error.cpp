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
	auto gt_mask = img::imread<float,1>(doc["gt_mask"].string().c_str());
    std::vector<std::map<std::string,json::value>> results;

    // sweep robust loss
    for(const auto & thresh : {0.5f, 0.75f, 1.0f, 2.0f, 3.0f}) {
        double err = 0.0;
        double count = 0.0;

		double err_n = 0.0;
		double count_n = 0.0;
        for(int i=0; i < disp.size(); i++) {
            auto isValid = gt_mask(i) > 0.5 ? true : false;
            if(!isValid)
                continue;
            err += std::min(thresh,std::abs(gt_disp(i) - disp(i)));
            count += 1;
			

            auto thinksValid = conf(i) > 0.5 ? true : false;
			if(!thinksValid)
				continue;
			
            err_n += std::min(thresh,std::abs(gt_disp(i) - disp(i)));
            count_n += 1;
		}
        err_n /= count_n ? count_n : 1;
        err /= count ? count : 1;
        printf("%.2f %.4f %.4f\n",thresh,err,err_n);
	}
    return 0;
}
