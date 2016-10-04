#include "r200Match.h"
#include <algorithm>
#include <limits>

using namespace stereo;

#ifdef _WIN32
#define popcount __popcnt
#else
#define popcount __builtin_popcount
#endif

// Census Radius and Width
#define C_R (3)
#define C_W (2 * C_R + 1)
#define DS (1)
// sampling pattern
// . X . X . X .
// X . X . X . X
// . X . X . X .
// X . X 0 X . X
// . X . X . X .
// X . X . X . X
// . X . X . X .

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

R200Match::R200Match(int w, int h, int d, int m)
    : StereoMatch(w, h, d, m)
    , costs(w * d * h)
    , censusLeft(w * h, 0)
    , censusRight(w * h, 0)
{
}

R200Match::R200Match(int w, int h, int maxdisp, const alg_config & cfg)
	: R200Match(w, h, maxdisp, cfg.dispmul)
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


template<typename TG, int CG>
std::vector<uint16_t> domainTransform(
	const std::vector<uint16_t>& input,
	img::Image<TG, CG> guide,
	const R200Match::alg_config & config
	) {
    auto ratio = config.dt_space / config.dt_range;
	const int DT_B_R = config.box_radius;
	auto width = guide.width;
	auto height = guide.height;
	auto C = input.size()/ (width*height) ;


    img::Image<float, 1> ctx(width, height,0.f);
    img::Image<float, 1> cty(width, height,0.f);
    img::Image<float, 1> f_tmp(width, height,0.f);

	std::vector<float> out(input.size());
	std::vector<uint16_t> out_final(input.size());

    for (int i = 0; i < width*height*C; i++)
        out[i] = static_cast<float>(input[i]);

    //ctx
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width - 1; x++) {
            auto idx = CG*(y*ctx.width + x);
            auto idxn = CG*(y*ctx.width + x + 1);
            auto idxm = (y*ctx.width + x);

            float sum = 0;
            for (int c = 0; c < CG; c++)
                sum += std::abs(guide.ptr[idx + c] - guide.ptr[idxn + c]);
            ctx.ptr[idxm] = 1.0f + ratio*sum;
        }
    }

    //cty
    for (int x = 0; x < width; x++) {
        float sum = 0;
        for (int y = 0; y < height - 1; y++) {
            auto idx = CG*(y*cty.width + x);
            auto idxn = CG*((y + 1)*cty.width + x);
            auto idxm = (y*ctx.width + x);

            float sum = 0;
            for (int c = 0; c < CG; c++)
                sum += std::abs(guide.ptr[idx + c] - guide.ptr[idxn + c]);
            cty.ptr[idxm] = 1.0f + ratio*sum;
        }
    }

    // apply recursive filtering
    for (int i = 0; i < config.dt_iter; i++) {
        auto sigma_H = config.dt_space * sqrt(3.0f) * pow(2.0f, config.dt_iter - i - 1) / sqrt(pow(4.0f, config.dt_iter) - 1);
        auto alpha = exp(-sqrt(2.0f) / sigma_H);
        //horiz pass
        //generate f
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width - 1; x++) {
                auto idx = y*width + x;
                f_tmp.ptr[idx] = pow(alpha, ctx.ptr[idx]);
            }
        }
        //apply
        for (int y = DT_B_R; y < height - DT_B_R; y++) {
            for (int x = DT_B_R +1; x < width- DT_B_R; x++) {
                auto idx = C*(y*width + x);
                auto idxp = C*(y*width + x - 1);
                auto idxpm = (y*width + x - 1);

                float a = f_tmp.ptr[idxpm];
                for (int c = 0; c < C; c++) {
                    float p = out[idx + c];
                    float pn = out[idxp + c];

                    out[idx + c] = p + a*(pn - p);
                }
            }
            for (int x = width - -DT_B_R -2; x >= DT_B_R; x--) {
                auto idx = C*(y*width + x);
                auto idxn = C*(y*width + x + 1);
                auto idxm = (y*width + x);

                float a = f_tmp.ptr[idxm];
                for (int c = 0; c < C; c++) {
                    float p = out[idx + c];
                    float pn = out[idxn + c];

                    out[idx + c] = p + a*(pn - p);
                }
            }
        }

        //vertical pass
        //generate f
        for (int y = DT_B_R; y < height - 1 - DT_B_R; y++) {
            for (int x = DT_B_R; x < width- DT_B_R; x++) {
                auto idx = y*width + x;
                f_tmp.ptr[idx] = pow(alpha, cty.ptr[idx]);
            }
        }
        //apply
        for (int x = DT_B_R; x < width- DT_B_R; x++) {
            for (int y = DT_B_R +1; y < height - DT_B_R; y++) {
                auto idx = C*(y*width + x);
                auto idxp = C*((y - 1)*width + x);
                auto idxpm = (y - 1)*width + x;

                float a = f_tmp.ptr[idxpm];
                for (int c = 0; c < C; c++) {
                    float p = out[idx + c];
                    float pn = out[idxp + c];

                    out[idx + c] = p + a*(pn - p);
                }
            }
            for (int y = height - 2 - DT_B_R; y >= DT_B_R; y--) {
                auto idx = C*(y*width + x);
                auto idxn = C*((y + 1)*width + x);
                auto idxm = y*width + x;

                float a = f_tmp.ptr[idxm];
                for (int c = 0; c < C; c++) {
                    float p = out[idx + c];
                    float pn = out[idxn + c];

                    out[idx + c] = p + a*(pn - p);
                }
            }
        }
    }
    for (int i = 0; i < width*height*C; i++)
        out_final[i] = (uint16_t)(config.dt_scale*out[i] + 0.5f);

    return out_final;
}

static float subpixel(float costLeft, float costMiddle, float costRight)
{
    if (costMiddle >= 0xfffe || costLeft >= 0xfffe || costRight >= 0xfffe)
        return 0.f;

    auto num = costRight - costLeft;
    auto den = (costRight < costLeft) ? (costMiddle - costLeft) : (costMiddle - costRight);
    return den != 0 ? 0.5f * (num / den) : 0;
}

void R200Match::match(img::Img<uint16_t>& left, img::Img<uint16_t>& right, img::Img<uint16_t>& disp, img::Img<uint8_t>& conf)
{
    auto lptr = left.data.get();
    auto rptr = right.data.get();
    auto dptr = disp.data.get();

	// old school?
	const auto B_R = config.box_radius;
    const auto default_cost = (config.score_max + config.dt_scale - 1) / config.dt_scale;
    censusTransform(lptr, censusLeft.data(), width, height);
    censusTransform(rptr, censusRight.data(), width, height);
    img::Img<uint32_t> lc(left.width, left.height, (uint32_t*)censusLeft.data());
    img::Img<uint32_t> rc(left.width, left.height, (uint32_t*)censusRight.data());
	std::fill(costs.begin(), costs.end(), default_cost);

    for (int y = B_R; y < height - B_R; y++) {
        //printf("\r %.2lf %%", 100.0*static_cast<double>(y) / static_cast<double>(height));
        auto costX = costs.data() + y * (width*maxdisp);
#pragma omp parallel for
        for (int x = B_R; x < width - B_R; x++) {
            auto lb = std::max(B_R, x - maxdisp);
            auto search_limit = x - lb;
            for (int d = 0; d < search_limit; d++) {
                uint16_t cost = 0;
                for (int i = -B_R; i <= B_R; i++) {
                    for (int j = -B_R; j <= B_R; j++) {
                        auto pl = censusLeft[(y + i) * width + (x + j)];
                        auto pr = censusRight[(y + i) * width + (x + j - (d - DS))];

                        cost += popcount(pl ^ pr);
                    }
                }
                costX[x * maxdisp + d] = cost;
            }
        }
    }
	if (config.domain_transform) {
		// input volume, edge image, iterations (3), X-Y Sigma, Value Sigma)
		costs = domainTransform(costs, left, config);
	}
    if(costsName.size()) {
        struct raw_header {int w,h,c,bpp;};
        raw_header hd = {width,height,maxdisp,2};
        std::ofstream outn(costsName,std::ofstream::binary);
        //std::vector<uint32_t> cw(costs.size());
        //for(int i=0; i < costs.size(); i++) {
        //    cw[i] = costs[i];
        //}
        outn.write((char*)&hd,sizeof(raw_header)); 
        outn.write((char*)costs.data(),costs.size()*sizeof(uint16_t));
    }
    for (int y = B_R; y < height - B_R; y++) {
        auto prevVal = 0;
        auto costX = costs.data() + y * (width*maxdisp);
#pragma omp parallel for
        for (int x = B_R; x < width - B_R; x++) {
            auto minRVal = std::numeric_limits<uint16_t>::max();
            auto minRIdx = 0;
            auto minLVal = std::numeric_limits<uint16_t>::max();
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
            uint16_t res = (uint16_t)std::round((std::max(0,minLIdx-DS)  + spL) * muldisp);
            uint16_t bitMask = 0;

            // left-right threshold
            bitMask |= (abs(minLIdx - minRIdx) <= config.left_right_int && abs(spR - spL) <= config.left_right_sub);

            // neighbor threshold
            auto diffL = (int)nL - (int)nC;
            auto diffR = (int)nR - (int)nC;
            bitMask |= (diffL >= config.neighbor || diffR >= config.neighbor) << 1;

            // second peak threshold
            auto minL2Val = std::numeric_limits<uint16_t>::max();
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
            int centerV = lptr[y*width + x];
            for (int i = -B_R; i <= B_R; i++) {
                for (int j = -B_R; j <= B_R; j++) {
                    int v = lptr[(y + i)*width + (x + j)];
                    tc += abs(centerV - v) > config.texture_diff ? 1 : 0;
                }
            }
            bitMask |= (tc >= config.texture_count) << 3;

            // score limits
            bitMask |= (minLVal >= config.score_min && minLVal <= config.score_max) << 4;


            // median threshold
            auto me = std::numeric_limits<uint16_t>::max();
            auto initialized = false;
            for (int d = 0; d < maxdisp; d++) {
                auto cost = costX[x * maxdisp + d];
                if (!initialized && cost != default_cost) {
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
            dptr[y * width + x] = res;
        }
    }
}
