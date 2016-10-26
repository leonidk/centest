#include "r200Match.h"
#include <algorithm>
#include <limits>
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
#include <numeric>

#define JSON_H_IMPLEMENTATION
#include "json.h"



static float subpixel(float costLeft, float costMiddle, float costRight)
{
    if (costMiddle >= 0xfffe || costLeft >= 0xfffe || costRight >= 0xfffe)
        return 0.f;

    auto num = costRight - costLeft;
    auto den = (costRight < costLeft) ? (costMiddle - costLeft) : (costMiddle - costRight);
    return den != 0 ? 0.5f * (num / den) : 0;
}
#define DS (1)

void generateDispConf(stereo::R200Match::alg_config & config, img::Img<uint16_t> & left, std::vector<uint32_t> & costs, img::Img<float> & disp, img::Img<float> & conf) 
{
    const auto B_R = config.box_radius;
    auto height = left.height;
    auto width = left.width;
    int maxdisp = costs.size()/(width*height);
    const auto default_score = (config.score_max + config.dt_scale - 1) / config.dt_scale;
    for (int y = B_R; y < height - B_R; y++) {
        auto prevVal = 0;
        auto costX = costs.data() + y * (width*maxdisp);
#pragma omp parallel for
        for (int x = B_R; x < width - B_R; x++) {
            auto minRVal = std::numeric_limits<uint32_t>::max();
            auto minRIdx = 0;
            auto minLVal = std::numeric_limits<uint32_t>::max();
            auto minLIdx = 0;
            for (int d = 0; d < maxdisp; d++) {
                auto cost = costX[x * maxdisp + d];
                if (cost < minLVal) {
                    minLVal = cost;
                    minLIdx = d;
                }
            }
            auto xl = std::max(0, x - minLIdx);
            auto xu = std::min(width - 1, xl + maxdisp);
            for (int xd = xl; xd < xu; xd++) {
                auto d = xd - x + minLIdx;
                auto cost = costX[xd * maxdisp + d];
                if (cost < minRVal) {
                    minRVal = cost;
                    minRIdx = d;
                }
            }
            // subpixel left
            auto nL = costX[x * maxdisp + std::max(minLIdx - 1, 0)];
            auto nC = costX[x * maxdisp + minLIdx];
            auto nR = costX[x * maxdisp + std::min(minLIdx + 1, maxdisp - 1)];
            auto spL = (minLIdx > 0 && minLIdx < maxdisp - 1) ? subpixel(nL, nC, nR) : 0;
            // subpixel right
            auto rL = costX[std::max(0, (x - 1)) * maxdisp + std::max(minLIdx - 1, 0)];
            auto rC = costX[(x)*maxdisp + minLIdx];
            auto rR = costX[std::min(width - 1, (x + 1)) * maxdisp + std::min(minLIdx + 1, maxdisp - 1)];
            auto spR = (minLIdx < maxdisp - 1) ? subpixel(rL, rC, rR) : 0;

            // disparity computation
            float res = (std::max(0,minLIdx-DS)  + spL);
            uint16_t bitMask = 0;

            // left-right threshold
            bitMask |= (abs(minLIdx - minRIdx) <= config.left_right_int && abs(spR - spL) <= config.left_right_sub);

            // neighbor threshold
            auto diffL = (int)nL - (int)nC;
            auto diffR = (int)nR - (int)nC;
            bitMask |= (diffL >= config.neighbor || diffR >= config.neighbor) << 1;

            // second peak threshold
            auto minL2Val = std::numeric_limits<uint32_t>::max();
            for (int d = 0; d < maxdisp; d++) {
                auto cost = costX[x * maxdisp + d];
                auto costNext = (d == maxdisp - 1) ? cost : costX[x * maxdisp + d+1];
                auto costPrev = (d == 0) ? cost : costX[x * maxdisp + d - 1];

                if (cost < costNext && cost < costPrev) {
                    if (d == minLIdx)
                        continue;
                    if (cost < minL2Val)
                        minL2Val = cost;
                }
            }
            auto diffSP = minL2Val - minLVal;
            bitMask |= (diffSP >= config.second_peak) << 2;

            // texture difference (waste of time?)
            auto tc = 0;
            int centerV = left(y,x);
            for (int i = -B_R; i <= B_R; i++) {
                for (int j = -B_R; j <= B_R; j++) {
                    int v = left(y + i,x + j);
                    tc += abs(centerV - v) > config.texture_diff ? 1 : 0;
                }
            }
            bitMask |= (tc >= config.texture_count) << 3;

            // score limits
            bitMask |= (minLVal >= config.score_min && minLVal <= config.score_max) << 4;


            // median threshold
            auto me = std::numeric_limits<uint32_t>::max();
            auto initialized = false;
            for (int d = 0; d < maxdisp; d++) {
                auto cost = costX[x * maxdisp + d];
                if (!initialized && cost != default_score) {
                    initialized = true;
                    me = cost;
                }
                if (cost > me)
                    me += config.median_plus;
                else if (cost < me)
                    me -= config.median_minus;
            }
            bitMask |= (me - minLVal >= config.median_thresh) << 5;

            //mask
            conf(y,x) = (bitMask == 0x3F) ? 1 : 0;
            // hole filling
            //if (config.hole_fill) {
            //    prevVal = res ? res : prevVal;
            //    res = res ? res : prevVal;
            //}

            // final set
            disp(y,x) = res;
        }
    }
}

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
	
	stereo::R200Match::alg_config cfg;

	if (doc["config"]["algorithm"].string() == "r200") {
		from_json(cfg, doc["config"]);
	} 
	auto costsName = doc["costs"].string();
    struct raw_header {int w,h,c,bpp;};
    raw_header hd;
    std::ifstream outn(costsName,std::ifstream::binary);
    outn.read((char*)&hd,sizeof(raw_header));
    std::vector<uint32_t> costs(hd.w*hd.h*hd.c); 
    if(hd.bpp == 4) {
        outn.read((char*)costs.data(),costs.size()*sizeof(uint32_t));
    } else if(hd.bpp == 2) {
        std::vector<uint16_t> costst(hd.w*hd.h*hd.c); 
        outn.read((char*)costst.data(),costst.size()*sizeof(uint16_t));
        for(int i=0; i < costs.size(); i++)
            costs[i] = costst[i];
    }
	img::Img<float> dispf(hd.w, hd.h, 0.f);
	img::Img<float> conff(hd.w, hd.h, 0.f);

	generateDispConf(cfg,left_g,costs,dispf,conff);

	img::imwrite(doc["output_disp"].string().c_str(), dispf);
	img::imwrite(doc["output_conf"].string().c_str(), conff);

    return 0;
}
