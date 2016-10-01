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
#include <iomanip>
#include <fstream>
#include <numeric>
#define JSON_H_IMPLEMENTATION
#include "json.h"
template <typename T>
std::string to_string_with_precision(const T a_value, const int n = 6)
{
    std::ostringstream out;
    out << std::setprecision(n) << a_value;
    return out.str();
}

std::string pretty_print(const json::value & v) { std::ostringstream ss; ss << tabbed(v, 4); return ss.str(); }

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
        auto str = std::accumulate(begin(args), end(args), std::string());
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

	auto disp = img::imread<float,1>(doc["output_disp"].string().c_str());
	auto conf = img::imread<float,1>(doc["output_conf"].string().c_str());

	auto gt_disp = img::imread<float,1>(doc["gt"].string().c_str());
	auto gt_mask = img::imread<float,1>(doc["gt_mask"].string().c_str());
    json::object results;

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

        json::object res;
        res["name"] = std::string("err_") +to_string_with_precision(thresh,3) ;
        res["result"] = err;
        res["threshold"] = thresh;
        res["description"] = json::value{std::string("Robust loss over all pixels with t=") + std::to_string(thresh)};
        results[res["name"].string()] = res;
        
        res["name"] = std::string("errn_") +to_string_with_precision(thresh,3) ;
        res["result"] = err_n;
        res["threshold"] = thresh;
        res["description"] = json::value{std::string("Robust loss over valid pixels with t=") + std::to_string(thresh)};
        results[res["name"].string()] = res;
	}

    // sweep f# scores
    for(const auto & beta : {0.25f, 0.5f, 1.0f, 2.0f, 4.0f}) 
    {
        double tp = 0.0f;
        double fp = 0.0f;
        double tn = 0.0f;
        double fn = 0.0f;
        for(int i=0; i < disp.size(); i++) {
            auto isValid = gt_mask(i) > 0.5 ? true : false;
            auto thinksValid = conf(i) > 0.5 ? true : false;
			
            if(isValid) {
                if(thinksValid)
                    tp++;
                else
                    fn++;
            } else {
                if(thinksValid)
                    fp++;
                else
                    tn++;
            }
        }
        auto b2 = (beta*beta);
        auto b2p1 = (b2 + 1.0);
        auto score = (b2p1*tp)/(b2p1*tp+b2*fn+fp);
        json::object res;
        res["name"] = std::string("f_") +to_string_with_precision(beta,3) ;
        res["result"] = score;
        res["beta"] = beta;
        res["description"] = json::value{std::string("F score with B=") + std::to_string(beta)};
        results[res["name"].string()] = res;
    }
    std::cout << pretty_print(results);
    //std::ofstream outputfile(doc["output"].string());
    //outputfile << pretty_print(results);
    return 0;
}
