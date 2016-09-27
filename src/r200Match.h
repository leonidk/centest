#pragma once

#include "stereo.h"
namespace stereo {

class R200Match : public StereoMatch {
public:
    using StereoMatch::match;
    // Using the parameters from
    // https://github.com/IntelRealSense/librealsense/blob/master/include/librealsense/rsutil.h

    struct alg_config {
		int dispmul = 4;
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
    R200Match(int w, int h, int d, int m);
	R200Match(int w, int h, int maxdisp, const alg_config & cfg);

    virtual void match(img::Img<uint16_t>& left, img::Img<uint16_t>& right,
		img::Img<uint16_t>& disp, img::Img<uint8_t>& conf) override;

private:
	alg_config config;
    std::vector<uint32_t> censusLeft, censusRight;
    std::vector<uint16_t> costs;
};
}
