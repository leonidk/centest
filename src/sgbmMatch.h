#pragma once

#include "stereo.h"
namespace stereo {

class sgbmMatch : public StereoMatch {
public:
    using StereoMatch::match;
	struct alg_config {
		int dispmul = 4;
		int box_radius = 3;
		int left_right_int = 1;
		float left_right_sub = 0.75;
		int neighbor = 20;
		int second_peak = 50;
		int texture_diff = 0;
		int texture_count = 0;
		int score_min = 0;
		int score_max = 20000;
		int median_plus = 10;
		int median_minus = 10;
		int median_thresh = 500;
		bool hole_fill = false;
		int cost_abs = 1;
		int cost_ham = 3;
		int p1 = 2000;
		int p2 = 8000;
		bool sgm = true;
		bool scale_p2 = true;
		bool use_blf = false;
		float blf_range = 25.0f;
		float blf_space = 2.25f;
	};
    sgbmMatch(int w, int h, int d, int m);
	sgbmMatch(int w, int h, int maxdisp, const alg_config & cfg);

    virtual void match(img::Img<uint16_t>& left, img::Img<uint16_t>& right,
		img::Img<uint16_t>& disp, img::Img<uint8_t>& conf) override;

private:
    std::vector<uint32_t> censusLeft, censusRight;
    std::vector<uint32_t> costs;
    std::vector<uint32_t> costsSummed;
	alg_config config;
};
}
