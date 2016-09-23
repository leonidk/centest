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

// folder target
// contains
//  * calib.txt
//  * disp0GT.pfm
//  * im0.png
//  * im1.png
//  * mask0nocc.png

struct alg_config {
	int dispmul = 4;
	std::string algorithm = "r200";
	int box_radius = 3;
	int left_right_int = 1;
	float left_right_sub = 0.75;
	int neighbor = 7;
	int second_peak = 10;
	int texture_diff = 4;
	int texture_count = 6;
	int score_min = 1;
	int score_max = 512;
	int median_plus = 5;
	int median_minus = 5;
	int median_thresh = 192;
	bool hole_fill = false;
	bool domain_transform = false;
	int dt_scale = 1;
	int dt_iter = 1;
	float dt_space = 10;
	float dt_range = 90;
};
template<class F> void visit_fields(alg_config & o, F f) {
	f("dispmul", o.dispmul);
	f("algorithm", o.algorithm);
	f("box_radius", o.box_radius);
	f("left_right_int", o.left_right_int);
	f("left_right_sub", o.left_right_sub);
	f("neighbor", o.neighbor);
	f("second_peak", o.second_peak);
	f("texture_diff", o.texture_diff);
	f("texture_count", o.texture_count);
	f("score_min", o.score_min);
	f("score_max", o.score_max);
	f("median_plus", o.median_plus);
	f("median_minus", o.median_minus);
	f("median_thresh", o.median_thresh);
	f("hole_fill", o.hole_fill);
	f("domain_transform", o.domain_transform);
	f("dt_scale", o.dt_scale);
	f("dt_iter", o.dt_iter);
	f("dt_space", o.dt_space);
	f("dt_range", o.dt_range);
}


const int MAXDISP = 70;
const int MULDISP = 4;
int main(int argc, char* argv[])
{
	alg_config cfg;
	json::value doc;
    if (argc < 2)
        return 1;
	if (auto in = std::ifstream(argv[1]))
	{
		std::string str((std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());
		doc = json::parse(str);
		from_json(cfg, doc["config"]);
	}
	else {
		return 1;
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
	img::Img<uint16_t> disp(left.width, left.height);
	
    memset(disp.data.get(), 0, left.width * left.height * sizeof(uint16_t));
	std::unique_ptr<stereo::StereoMatch> cm(nullptr);
	if (cfg.algorithm == "r200") {
		cm = std::make_unique<stereo::R200Match>(left.width, left.height, MAXDISP, MULDISP);
	}
	else {

	}
    //stereo::sgbmMatch cm(;


    ////for debug
    //for (int i = 0; i < gt.width * gt.height; i++) {
    //    gt.ptr[i] = (mask.ptr[i] != 255) ? std::numeric_limits<float>::infinity() : gt.ptr[i];
    //}

    //auto startTime = std::chrono::steady_clock::now();
    //cm.match(left_g, right_g,gt,disp);
    //auto endTime = std::chrono::steady_clock::now();;

    //auto ot = img::Image<uint8_t, 1>(disp.width, disp.height);
    //auto dptr = disp.data.get();
    //auto optr = ot.data.get();
    //auto mptr = mask.data.get();

    //memset(optr, 0, disp.width * disp.height);

    //for (int i = 0; i < ot.width * ot.height; i++) {
    //    optr[i] = (uint8_t)std::round(dptr[i] * 254.0 / (static_cast<double>(cm.maxdisp * cm.muldisp)));
    //}
    //double mse = 0;
    //double count = 0;
    //auto sqr = [](double a) { return a * a; };
    //auto gptr = gt.data.get();

    //for (int i = 0; i < ot.width * ot.height; i++) {
    //    double pred = dptr[i] / static_cast<double>(cm.muldisp);
    //    double corr = gptr[i];
    //    uint8_t msk = mptr[i];

    //    if (std::isfinite(corr) && msk == 255) {
    //        mse += sqr(pred - corr);
    //        count++;
    //    }
    //}
    //
    //img::imwrite("disp-out.png", ot);
    //printf("\n\n %ld seconds\n", std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
    //printf("\n\n %lf average error\n", sqrt(mse / count));

    return 0;
}
