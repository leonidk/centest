#include "r200Match.h"
#include <algorithm>
#include <limits>

using namespace stereo;

// Using the parameters from
// https://github.com/IntelRealSense/librealsense/blob/master/include/librealsense/rsutil.h

// Census Radius and Width
#define C_R (3)
#define C_W (2 * C_R + 1)

// Box Filter Radius and Width
#define B_R (0)
#define B_W (2 * B_R + 1)

// Left-Right Threshold
// Dependent on subpixel algorithm. May not work well with subpixel
#define LRT (1)
#define LRS (0.75f)

// Neighbor Threshold
#define NT (7)

// Second Peak
#define SP (10)

// Texture Diff (commented out below)
#define TD (4)
#define TC (6)

// Score Limits
#define SMIN (1)
#define SMAX (512)

// Median
#define MP (5)
#define MM (5)
#define MT (192)

// optional hole filling for 100% density
// not on r200
#define HOLE_FILL (0)

// domain transform
#if B_R == 0
#undef NT
#undef SP
#undef SMIN
#undef SMAX
#undef MP
#undef MM
#undef MT
#define NT (0)
#define SP (0)
#define SMIN (0)
#define SMAX (512)
#define MP (5)
#define MM (5)
#define MT (50)


#define USE_DT (1)
#define DT_SCALE (49)
#define DT_ITER   (3)
//
////DS4-like
//#define DT_SPACE (3)
//#define DT_RANGE (USHRT_MAX)

// better mode?
//#define DT_SPACE (10)
//#define DT_RANGE (60)

// blob mode
#define DT_SPACE (30)
#define DT_RANGE (USHRT_MAX)

// better mode?
//#define DT_SPACE (USHRT_MAX)
//#define DT_RANGE (30)

#endif

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

static void censusTransform(uint8_t* in, uint32_t* out, int w, int h)
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

template<typename T, int C, typename TG, int CG>
img::Image<T, C> domainTransform(
    img::Image<T, C> input,
    img::Image<TG, CG> guide,
    const int iters,
    const float sigma_space,
    const float sigma_range) {
    auto ratio = sigma_space / sigma_range;

    img::Image<float, 1> ctx(input.width, input.height);
    img::Image<float, 1> cty(input.width, input.height);
    img::Image<float, 1> f_tmp(input.width, input.height);

    img::Image<float, C> out(input.width, input.height);
    img::Image<T, C> out_final(input.width, input.height);

    for (int i = 0; i < input.width*input.height*C; i++)
        out.ptr[i] = static_cast<float>(input.ptr[i]);

    //ctx
    for (int y = 0; y < input.height; y++) {
        for (int x = 0; x < input.width - 1; x++) {
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
    for (int x = 0; x < input.width; x++) {
        float sum = 0;
        for (int y = 0; y < input.height - 1; y++) {
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
    for (int i = 0; i < iters; i++) {
        auto sigma_H = sigma_space * sqrt(3.0f) * pow(2.0f, iters - i - 1) / sqrt(pow(4.0f, iters) - 1);
        auto alpha = exp(-sqrt(2.0f) / sigma_H);
        //horiz pass
        //generate f
        for (int y = 0; y < input.height; y++) {
            for (int x = 0; x < input.width - 1; x++) {
                auto idx = y*input.width + x;
                f_tmp.ptr[idx] = pow(alpha, ctx.ptr[idx]);
            }
        }
        //apply
        for (int y = 0; y < input.height; y++) {
            for (int x = 1; x < input.width; x++) {
                auto idx = C*(y*input.width + x);
                auto idxp = C*(y*input.width + x - 1);
                auto idxpm = (y*input.width + x - 1);

                float a = f_tmp.ptr[idxpm];
                for (int c = 0; c < C; c++) {
                    float p = out.ptr[idx + c];
                    float pn = out.ptr[idxp + c];

                    out.ptr[idx + c] = p + a*(pn - p);
                }
            }
            for (int x = input.width - 2; x >= 0; x--) {
                auto idx = C*(y*input.width + x);
                auto idxn = C*(y*input.width + x + 1);
                auto idxm = (y*input.width + x);

                float a = f_tmp.ptr[idxm];
                for (int c = 0; c < C; c++) {
                    float p = out.ptr[idx + c];
                    float pn = out.ptr[idxn + c];

                    out.ptr[idx + c] = p + a*(pn - p);
                }
            }
        }

        //vertical pass
        //generate f
        for (int y = 0; y < input.height - 1; y++) {
            for (int x = 0; x < input.width; x++) {
                auto idx = y*input.width + x;
                f_tmp.ptr[idx] = pow(alpha, cty.ptr[idx]);
            }
        }
        //apply
        for (int x = 0; x < input.width; x++) {
            for (int y = 1; y < input.height; y++) {
                auto idx = C*(y*input.width + x);
                auto idxp = C*((y - 1)*input.width + x);
                auto idxpm = (y - 1)*input.width + x;

                float a = f_tmp.ptr[idxpm];
                for (int c = 0; c < C; c++) {
                    float p = out.ptr[idx + c];
                    float pn = out.ptr[idxp + c];

                    out.ptr[idx + c] = p + a*(pn - p);
                }
            }
            for (int y = input.height - 2; y >= 0; y--) {
                auto idx = C*(y*input.width + x);
                auto idxn = C*((y + 1)*input.width + x);
                auto idxm = y*input.width + x;

                float a = f_tmp.ptr[idxm];
                for (int c = 0; c < C; c++) {
                    float p = out.ptr[idx + c];
                    float pn = out.ptr[idxn + c];

                    out.ptr[idx + c] = p + a*(pn - p);
                }
            }
        }
    }
    for (int i = 0; i < input.width*input.height*C; i++)
        out_final.ptr[i] = (T)(DT_SCALE*out.ptr[i] + 0.5f);

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

void R200Match::match(img::Img<uint8_t>& left, img::Img<uint8_t>& right, img::Img<uint16_t>& disp)
{
    auto lptr = left.data.get();
    auto rptr = right.data.get();
    auto dptr = disp.data.get();

    censusTransform(lptr, censusLeft.data(), width, height);
    censusTransform(rptr, censusRight.data(), width, height);
    img::Img<uint32_t> lc(left.width, left.height, (uint32_t*)censusLeft.data());
    img::Img<uint32_t> rc(left.width, left.height, (uint32_t*)censusRight.data());
    //img::Img<uint16_t> costI(maxdisp, width, (uint16_t*)costs.data());
    std::fill(costs.begin(), costs.end(), SMAX);
    img::Img<uint16_t> interesting(left.width, left.height);
    memset(interesting.ptr, 0, left.width*left.height * 2);

    for (int y = B_R; y < height - B_R; y++) {
        //printf("\r %.2lf %%", 100.0*static_cast<double>(y) / static_cast<double>(height));
        //#pragma omp parallel for
        auto costX = costs.data() + y * (width*maxdisp);
        for (int x = B_R; x < width - B_R; x++) {
            auto lb = std::max(B_R, x - maxdisp);
            auto search_limit = x - lb;
            for (int d = 0; d < search_limit; d++) {
                uint16_t cost = 0;
                for (int i = -B_R; i <= B_R; i++) {
                    for (int j = -B_R; j <= B_R; j++) {
                        auto pl = censusLeft[(y + i) * width + (x + j)];
                        auto pr = censusRight[(y + i) * width + (x + j - d)];

                        cost += popcount(pl ^ pr);
                    }
                }
                costX[x * maxdisp + d] = cost;
            }
        }
    }
#if USE_DT
    img::Image<uint16_t, 64> costImage(left.width, left.height, (uint16_t*)costs.data());
    // input volume, edge image, iterations (3), X-Y Sigma, Value Sigma)
    auto costsNew = domainTransform(costImage, left, DT_ITER, DT_SPACE, DT_RANGE);
    img::Image<uint16_t, 64> costImageF(left.width, left.height, (uint16_t*)costsNew.ptr);
    memcpy(costs.data(), costsNew.ptr, sizeof(uint16_t)*left.width*left.height * 64);
#endif
    for (int y = B_R; y < height - B_R; y++) {
        auto prevVal = 0;
        auto costX = costs.data() + y * (width*maxdisp);
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
            auto spR = (minLIdx > 0 && minLIdx < maxdisp - 1) ? subpixel(rL, rC, rR) : 0;

            // disparity computation
            uint16_t res = (uint16_t)std::round((minLIdx + spL) * muldisp);
            uint16_t bitMask = 0;

            // left-right threshold
            bitMask |= (abs(minLIdx - minRIdx) <= LRT && abs(spR - spL) <= LRS);

            // neighbor threshold
            auto diffL = (int)nL - (int)nC;
            auto diffR = (int)nR - (int)nC;
            bitMask |= (diffL >= NT || diffR >= NT) << 1;

            // second peak threshold
            auto minL2Val = std::numeric_limits<uint16_t>::max();
            for (int d = 0; d < maxdisp; d++) {
                auto cost = costX[x * maxdisp + d];
                if (d == minLIdx || d == minLIdx + 1 || d == minLIdx - 1)
                    continue;
                if (cost < minL2Val)
                    minL2Val = cost;
            }
            auto diffSP = minL2Val - minLVal;
            bitMask |= (diffSP >= SP) << 2;

            // texture difference (waste of time?)
            //auto tc = 0;
            //int centerV = lptr[y*width + x];
            //for (int i = -B_R; i <= B_R; i++) {
            //    for (int j = -B_R; j <= B_R; j++) {
            //        int v = lptr[(y + i)*width + (x + j)];
            //        tc += abs(centerV - v) > TD ? 1 : 0;
            //    }
            //}
            //bitMask |= (tc >= TC) << 3;

            // score limits
            bitMask |= (minLVal >= SMIN && minLVal <= SMAX) << 4;


            // median threshold
            auto me = std::numeric_limits<uint16_t>::max();
            auto initialized = false;
            for (int d = 0; d < maxdisp; d++) {
                auto cost = costX[x * maxdisp + d];
                if (!initialized && cost != std::numeric_limits<uint16_t>::max()) {
                    initialized = true;
                    me = cost;
                }
                if (cost > me)
                    me += MP;
                else if (cost < me)
                    me -= MM;
            }
            bitMask |= (me - minLVal >= MT) << 5;

            //mask
            res = (bitMask == 0x37) ? res : 0;
            interesting.ptr[y*width + x] = bitMask;
            // hole filling
            if (HOLE_FILL) {
                prevVal = res ? res : prevVal;
                res = res ? res : prevVal;
            }

            // final set
            dptr[y * width + x] = res;
        }
    }
}
