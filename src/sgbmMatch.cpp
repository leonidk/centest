#include "sgbmMatch.h"
#include <algorithm>
#include <limits>
#include <fstream>
#include <cmath>
using namespace stereo;

// Using the parameters from
// https://github.com/IntelRealSense/librealsense/blob/master/include/librealsense/rsutil.h


// sampling pattern
// . X . X . X .
// X . X . X . X
// . X . X . X .
// X . X 0 X . X
// . X . X . X .
// X . X . X . X
// . X . X . X .
#define C_R (3)
#define C_W (2 * C_R + 1)
#define DS (1)
// y,x
const int samples[] = {
    -3, -2,
    -3, 0,
    -3, 2,
    -2, -3,
    -2, -1,
    -2, 1,
    -2, 3,
    -1, -2,
    -1, 0,
    -1, 2,
    0, -3,
    0, -1,
    0, 1,
    0, 3,
    1, -2,
    1, 0,
    1, 2,
    2, -3,
    2, -1,
    2, 1,
    2, 3,
    3, -2,
    3, 0,
    3, 2
};


sgbmMatch::sgbmMatch(int w, int h, int d, int m)
    : StereoMatch(w, h, d, m)
    , costs(w * d)
    , costsSummed(w * d)
    , censusLeft(w * h, 0)
    , censusRight(w * h, 0)
{
}
sgbmMatch::sgbmMatch(int w, int h, int maxdisp, const alg_config & cfg)
	: sgbmMatch(w, h, maxdisp, cfg.dispmul)
{
	config = cfg;
}

static void censusTransform(uint16_t* in, uint32_t* out, int w, int h)
{
    int ns = (int)(sizeof(samples) / sizeof(int)) / 2;
    for (int y = C_R; y < h - C_R; y++) {
        for (int x = C_R; x < w - C_R; x++) {
            uint32_t px = 0;
            auto center = in[y * w + x];
            for (int p = 0; p < ns; p++) {
                auto yp = (y + samples[2 * p]);
                auto xp = (x + samples[2 * p + 1]);
                px |= (in[yp * w + xp] > center) << p;
            }
            out[y * w + x] = px;
        }
    }
}

#ifdef _WIN32
#define popcount __popcnt
#else
#define popcount __builtin_popcount
#endif
static float subpixel(float costLeft, float costMiddle, float costRight)
{
    if (costMiddle >= 0xfffe || costLeft >= 0xfffe || costRight >= 0xfffe)
        return 0.f;

    auto num = costRight - costLeft;
    auto den = (costRight < costLeft) ? (costMiddle - costLeft) : (costMiddle - costRight);
    return den != 0 ? 0.5f * (num / den) : 0;
}


void sgbmMatch::match(img::Img<uint16_t>& left, img::Img<uint16_t>& right,img::Img<uint16_t>& disp, img::Img<uint8_t>& conf)
{
    auto lptr = left.data.get();
    auto rptr = right.data.get();
    auto dptr = disp.data.get();

	auto box_width = config.box_radius * 2 + 1;

    censusTransform(lptr, censusLeft.data(), width, height);
    censusTransform(rptr, censusRight.data(), width, height);
    img::Img<uint32_t> lc(left.width, left.height, (uint32_t*)censusLeft.data());
    img::Img<uint32_t> rc(left.width, left.height, (uint32_t*)censusRight.data());
    img::Img<uint32_t> costI(maxdisp, width, (uint32_t*)costs.data());
    const auto default_score = config.score_max;
    std::fill(costsSummed.begin(), costsSummed.end(), default_score);

    std::vector<int32_t> topCosts(width * maxdisp, default_score);
    std::vector<int32_t> topLeftCosts(width * maxdisp, default_score);
    std::vector<int32_t> topRightCosts(width * maxdisp, default_score);
    std::vector<float> bilateralWeights(box_width * box_width);

    
    for (int y = config.box_radius; y < height - config.box_radius; y++) {
        //printf("\r %.2lf %%", 100.0*static_cast<double>(y) / static_cast<double>(height));
        auto prevVal = 0;
        std::fill(costs.begin(),costs.end(), default_score);
        if (config.use_blf) {
            #pragma omp parallel for
            for (int x = config.box_radius; x < width - config.box_radius; x++) {
                auto lb = std::max(config.box_radius, x - maxdisp);
                auto search_limit = x - lb;
                float middlePx = lptr[(y)*width + (x)];
                float blW = 0;
                for (int i = -config.box_radius; i <= config.box_radius; i++) {
                    for (int j = -config.box_radius; j <= config.box_radius; j++) {
                        float px = lptr[(y + i) * width + (x + j)];
                        auto rw = expf(-(px - middlePx) / (2.f * config.blf_range));
                        auto sw = expf(-sqrtf((float)(i * i + j * j)) / (2.f * config.blf_space));
                        bilateralWeights[(i + config.box_radius) * box_width + (j + config.box_radius)] = rw * sw;
                        blW += rw * sw;
                    }
                }

                for (int d = 0; d < search_limit; d++) {
                    float cost = 0;
                    for (int i = -config.box_radius; i <= config.box_radius; i++) {
                        for (int j = -config.box_radius; j <= config.box_radius; j++) {
                            auto pl = censusLeft[(y + i) * width + (x + j)];
                            auto pr = censusRight[(y + i) * width + (x + j - (d-DS))];

                            auto al = lptr[(y + i) * width + (x + j)];
                            auto ar = rptr[(y + i) * width + (x + j - (d-DS))];

                            cost += bilateralWeights[(i + config.box_radius) * box_width + (j + config.box_radius)] * (config.cost_ham * popcount(pl ^ pr) + config.cost_abs * abs(al - ar));
                        }
                    }
                    costs[x * maxdisp + d] = (uint32_t)std::round((box_width * box_width) * cost / blW);
                }
            }
        } else {
            #pragma omp parallel for
            for (int x = config.box_radius; x < width - config.box_radius; x++) {
                auto lb = std::max(config.box_radius, x - maxdisp);
                auto search_limit = x - lb;
                for (int d = 0; d < search_limit; d++) {
                    uint16_t cost = 0;
                    for (int i = -config.box_radius; i <= config.box_radius; i++) {
                        for (int j = -config.box_radius; j <= config.box_radius; j++) {
                            auto pl = censusLeft[(y + i) * width + (x + j)];
                            auto pr = censusRight[(y + i) * width + (x + j - (d-DS))];

                            auto al = lptr[(y + i) * width + (x + j)];
                            auto ar = rptr[(y + i) * width + (x + j - (d-DS))];

                            cost += config.cost_ham * popcount(pl ^ pr) + config.cost_abs * abs(al - ar);
                        }
                    }
                    costs[x * maxdisp + d] = cost;
                }
            }
        }
        auto clipD = [&](int d) { return std::min(maxdisp - 1, std::max(0, d)); };

        //  semiglobal
        if (config.sgm) {
            std::vector<int32_t> leftCosts(width * maxdisp, default_score);
            std::vector<int32_t> rightCosts(width * maxdisp, default_score);

            //left
            for (int x = 1; x < width; x++) {
                auto p1 =config.p1;
                auto p2 =config.p2;
                if (config.scale_p2) {
                    auto grad = abs(lptr[y * width + x - 1] - lptr[y * width + x]) + 1;
                    p2 = (p2 + grad - 1) / grad;
                    p1 = std::min(p1, p2 - 1);
                }
                auto lftI = 0;
                auto lftV = default_score;
                for (int d = 0; d < maxdisp; d++) {
                    auto cost = leftCosts[(x - 1) * maxdisp + d];
                    if (cost < lftV) {
                        lftV = cost;
                        lftI = d;
                    }
                }
                for (int d = 0; d < maxdisp; d++) {
                    //left
                    int lftcost = leftCosts[(x - 1) * maxdisp + d];
                    int lftcostP1 = std::min(leftCosts[(x - 1) * maxdisp + clipD(lftI + 1)],
                                        leftCosts[(x - 1) * maxdisp + clipD(lftI - 1)])
                        + p1;
                    int lftcostP2 = lftV + p2;
                    int lftC = (int)std::min({ lftcost, lftcostP1, lftcostP2 }) - (int)lftV;

                    leftCosts[x * maxdisp + d] = costs[x * maxdisp + d] + lftC;
                }
            }
            //top
            for (int x = 0; x < width; x++) {
                auto p1 =config.p1;
                auto p2 =config.p2;
                if (config.scale_p2) {
                    auto grad = abs(lptr[(y - 1) * width + x] - lptr[y * width + x]) + 1;
                    p2 = (p2 + grad - 1) / grad;
                    p1 = std::min(p1, p2 - 1);
                }
                auto topI = 0;
                auto topV = default_score;
                for (int d = 0; d < maxdisp; d++) {
                    auto cost = topCosts[(x)*maxdisp + d];
                    if (cost < topV) {
                        topV = cost;
                        topI = d;
                    }
                }
                for (int d = 0; d < maxdisp; d++) {
                    int topcost = topCosts[(x)*maxdisp + d];
                    int topcostP1 = std::min(topCosts[(x)*maxdisp + clipD(topI + 1)],
                                        topCosts[(x)*maxdisp + clipD(topI - 1)])
                        + p1;
                    int topcostP2 = topV + p2;
                    int topC = (int)std::min({ topcost, topcostP1, topcostP2 }) - (int)topV;

                    topCosts[x * maxdisp + d] = costs[x * maxdisp + d] + topC;
                }
            }
            // topleft
            for (int x = 1; x < width; x++) {
                auto p1 =config.p1;
                auto p2 =config.p2;
                if (config.scale_p2) {
                    auto grad = abs(lptr[(y - 1) * width + x - 1] - lptr[y * width + x]) + 1;
                    p2 = (p2 + grad - 1) / grad;
                    p1 = std::min(p1, p2 - 1);
                }
                auto tplI = 0;
                auto tplV = default_score;
                for (int d = 0; d < maxdisp; d++) {
                    auto cost = topLeftCosts[(x - 1) * maxdisp + d];
                    if (cost < tplV) {
                        tplV = cost;
                        tplI = d;
                    }
                }
                for (int d = 0; d < maxdisp; d++) {
                    //left
                    int tplcost = topLeftCosts[(x - 1) * maxdisp + d];
                    int tplcostP1 = std::min(topLeftCosts[(x - 1) * maxdisp + clipD(tplI + 1)],
                                        topLeftCosts[(x - 1) * maxdisp + clipD(tplI - 1)])
                        + p1;
                    int tplcostP2 = tplV + p2;
                    int tplC = (int)std::min({ tplcost, tplcostP1, tplcostP2 }) - (int)tplV;

                    topLeftCosts[x * maxdisp + d] = costs[x * maxdisp + d] + tplC;
                }
            }
            //right
            for (int x = width - 2; x >= 0; x--) {
                auto p1 =config.p1;
                auto p2 =config.p2;
                if (config.scale_p2) {
                    auto grad = abs(lptr[(y)*width + x + 1] - lptr[y * width + x]) + 1;
                    p2 = (p2 + grad - 1) / grad;
                    p1 = std::min(p1, p2 - 1);
                }
                auto rgtI = 0;
                auto rgtV = default_score;
                for (int d = 0; d < maxdisp; d++) {
                    auto cost = rightCosts[(x + 1) * maxdisp + d];
                    if (cost < rgtV) {
                        rgtV = cost;
                        rgtI = d;
                    }
                }
                for (int d = 0; d < maxdisp; d++) {
                    //left
                    int rgtcost = rightCosts[(x + 1) * maxdisp + d];
                    int rgtcostP1 = std::min(rightCosts[(x + 1) * maxdisp + clipD(rgtI + 1)],
                                        rightCosts[(x + 1) * maxdisp + clipD(rgtI - 1)])
                        + p1;
                    int rgtcostP2 = rgtV + p2;
                    int rgtC = (int)std::min({ rgtcost, rgtcostP1, rgtcostP2 }) - (int)rgtV;

                    rightCosts[x * maxdisp + d] = costs[x * maxdisp + d] + rgtC;
                }
            }
            //topRight
            for (int x = width - 2; x >= 0; x--) {
                auto p1 =config.p1;
                auto p2 =config.p2;
                if (config.scale_p2) {
                    auto grad = abs(lptr[(y - 1) * width + x + 1] - lptr[y * width + x]) + 1;
                    p2 = (p2 + grad - 1) / grad;
                    p1 = std::min(p1, p2 - 1);
                }
                auto rgtI = 0;
                auto rgtV = default_score;
                for (int d = 0; d < maxdisp; d++) {
                    auto cost = topRightCosts[(x + 1) * maxdisp + d];
                    if (cost < rgtV) {
                        rgtV = cost;
                        rgtI = d;
                    }
                }
                for (int d = 0; d < maxdisp; d++) {
                    //left
                    int rgtcost = topRightCosts[(x + 1) * maxdisp + d];
                    int rgtcostP1 = std::min(topRightCosts[(x + 1) * maxdisp + clipD(rgtI + 1)],
                                        topRightCosts[(x + 1) * maxdisp + clipD(rgtI - 1)])
                        + p1;
                    int rgtcostP2 = rgtV + p2;
                    int rgtC = (int)std::min({ rgtcost, rgtcostP1, rgtcostP2 }) - (int)rgtV;

                    topRightCosts[x * maxdisp + d] = costs[x * maxdisp + d] + rgtC;
                }
            }
            for (int x = 0; x < width; x++) {
                for (int d = 0; d < maxdisp; d++) {
                    costsSummed[x * maxdisp + d] = leftCosts[x * maxdisp + d]
                        + topLeftCosts[x * maxdisp + d]
                        + topCosts[x * maxdisp + d]
                        + topRightCosts[x * maxdisp + d]
                        + rightCosts[x * maxdisp + d];
                }
            }
        } else {
            for (int x = 0; x < width; x++) {
                for (int d = 0; d < maxdisp; d++) {
                    costsSummed[x * maxdisp + d] = costs[x * maxdisp + d];
                }
            }
        }
        if(costsName.size()) {
            struct raw_header {int w,h,c,bpp;};
            raw_header hd = {width,height,maxdisp,4};
            std::ofstream outn(costsName,std::ofstream::binary);
            outn.write((char*)&hd,sizeof(raw_header)); 
            outn.write((char*)costsSummed.data(),costsSummed.size()*sizeof(uint32_t));
        }
        //min selection
        for (int x = config.box_radius; x < width - config.box_radius; x++) {
            auto minRVal = (uint32_t)default_score;
            auto minRIdx = 0;
            auto minLVal = (uint32_t)default_score;
            auto minLIdx = 0;
            for (int d = 0; d < maxdisp; d++) {
                auto cost = costsSummed[x * maxdisp + d];
                if (cost < minLVal) {
                    minLVal = cost;
                    minLIdx = d;
                }
            }
            auto xl = std::max(0, x - minLIdx);
            auto xu = std::min(width - 1, xl + maxdisp);
            for (int xd = xl; xd < xu; xd++) {
                auto d = xd - x + minLIdx;
                auto cost = costsSummed[xd * maxdisp + d];
                if (cost < minRVal) {
                    minRVal = cost;
                    minRIdx = d;
                }
            }
            // subpixel left
            auto nL = costsSummed[x * maxdisp + std::max(minLIdx - 1, 0)];
            auto nC = costsSummed[x * maxdisp + minLIdx];
            auto nR = costsSummed[x * maxdisp + std::min(minLIdx + 1, maxdisp - 1)];
            auto spL = (minLIdx < maxdisp - 1) ? subpixel((float)nL, (float)nC, (float)nR) : 0;
            // subpixel right
            auto rL = costsSummed[(x - 1) * maxdisp + std::max(minLIdx - 1, 0)];
            auto rC = costsSummed[(x)*maxdisp + minLIdx];
            auto rR = costsSummed[(x + 1) * maxdisp + std::min(minLIdx + 1, maxdisp - 1)];
            auto spR = (minLIdx < maxdisp - 1) ? subpixel((float)rL, (float)rC, (float)rR) : 0;

            // disparity computation
            uint16_t res = (uint16_t)std::round((std::max(0,minLIdx-DS) + spL) * muldisp);
			uint16_t bitMask = 0;

            // left-right threshold
			bitMask |= (abs(minLIdx - minRIdx) <= config.left_right_int && abs(spR - spL) <= config.left_right_sub);

            // neighbor threshold
            auto diffL = (int)nL - (int)nC;
            auto diffR = (int)nR - (int)nC;
			bitMask |= (diffL >= config.neighbor || diffR >= config.neighbor) << 1;

            // second peak threshold
            uint32_t minL2Val = default_score;
            for (int d = 0; d < maxdisp; d++) {
                auto cost = costsSummed[x * maxdisp + d];
                auto costNext = (d == maxdisp - 1) ? cost : costsSummed[x * maxdisp + d + 1];
                auto costPrev = (d == 0) ? cost : costsSummed[x * maxdisp + d - 1];

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
            int centerV = lptr[y*width + x];
            for (int i = -config.box_radius; i <= config.box_radius; i++) {
                for (int j = -config.box_radius; j <= config.box_radius; j++) {
                    int v = lptr[(y + i)*width + (x + j)];
                    tc += abs(centerV - v) > config.texture_diff ? 1 : 0;
                }
            }
			bitMask |= (tc >= config.texture_count) << 3;

            // score limits
			bitMask |= (minLVal >= config.score_min && minLVal <= config.score_max) << 4;

            // median threshold
            uint32_t me = default_score;
            auto initialized = false;
            for (int d = 0; d < maxdisp; d++) {
                auto cost = costsSummed[x * maxdisp + d];
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
			conf(y, x) = (bitMask == 0x3F) ? 1 : 0;

            // hole filling
            //if (MOVE_LEFT) {
            //    prevVal = res ? res : prevVal;
            //    res = res ? res : prevVal;
            //}
            // final set
            dptr[y * width + x] = res;
        }
    }
}
