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

template<class F> void visit_fields(stereo::R200Match::alg_config & o, F f) {
	f("dispmul", o.dispmul);
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
template<class F> void visit_fields(stereo::sgbmMatch::alg_config & o, F f) {
	f("dispmul", o.dispmul);
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
	f("cost_abs", o.cost_abs);
	f("cost_ham", o.cost_ham);
	f("p1", o.p1);
	f("p2", o.p2);
	f("sgm", o.sgm);
	f("scale_p2", o.scale_p2);
	f("use_blf", o.use_blf);
	f("blf_range", o.blf_range);
	f("blf_space", o.blf_space);
}
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

	std::unique_ptr<stereo::StereoMatch> cm(nullptr);
	float scale_disp = 4.f;
	float scale_conf = 1.f;
	if (doc["config"]["algorithm"].string() == "r200") {
		stereo::R200Match::alg_config cfg;
		from_json(cfg, doc["config"]);
		cm = std::make_unique<stereo::R200Match>(left.width, left.height,doc["maxdisp"].number<int>(),cfg);
		scale_disp = (float)cfg.dispmul;
	} else if (doc["config"]["algorithm"].string() == "sgbm") {
		stereo::sgbmMatch::alg_config cfg;
		from_json(cfg, doc["config"]);
		cm = std::make_unique<stereo::sgbmMatch>(left.width, left.height, doc["maxdisp"].number<int>(), cfg);
		scale_disp = (float)cfg.dispmul;
	} else {
		cm = std::make_unique<stereo::R200Match>(left.width, left.height, doc["maxdisp"].number<int>(), (int)scale_disp);
	}
    auto startTime = std::chrono::steady_clock::now();
	auto res = cm->match(left_g, right_g);
	auto disp = res.first;
	auto conf = res.second;
    auto endTime = std::chrono::steady_clock::now();

	img::Img<float> dispf(disp.width, disp.height, 0.f);
	img::Img<float> conff(conf.width, conf.height, 0.f);

	for (int i = 0; i < disp.width*disp.height; i++)
		dispf(i) = static_cast<float>(disp(i)) / scale_disp;
	for (int i = 0; i < conf.width*conf.height; i++)
		conff(i) = static_cast<float>(conf(i)) / scale_conf;

	img::imwrite(doc["output_disp"].string().c_str(), dispf);
	img::imwrite(doc["output_conf"].string().c_str(), conff);


    return 0;
}
