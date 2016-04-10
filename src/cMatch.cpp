#include "cMatch.h"
#include <limits>
#include <algorithm>

using namespace stereo;

// Census Radius and Width
#define     C_R     (3)
#define     C_W     (2*C_R+1)

// Box Filter Radius and Width
#define     B_R     (3)
#define     B_W     (2*B_R+1)

// Left-Right Threshold
#define     LRT     (2)

// y,x
const int samples[] =
{
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


CensusMatch::CensusMatch(int w, int h, int d, int m)
    : StereoMatch(w, h, d, m), costs(w*d), censusLeft(w*h, 0), censusRight(w*h, 0)
{
}

static void censusTransform(uint8_t * in, uint32_t * out, int w, int h)
{
    int ns = (int)(sizeof(samples) / sizeof(int)) / 2;
    for (int y = C_R; y < h - C_R; y++) {
        for (int x = C_R; x < w - C_R; x++) {
            uint32_t px = 0;
            auto center = in[y*w + x];
            for (int p = 0; p < ns; p++) {
                auto yp = (y + samples[2 * p]);
                auto xp = (x + samples[2 * p + 1]);
                px |= (in[yp*w + xp] > center) << p;
            }
            out[y*w + x] = px;
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
    return den != 0 ? 0.5f*(num / den) : 0;
}

void CensusMatch::match(img::Img<uint8_t> & left, img::Img<uint8_t> & right, img::Img<uint16_t> & disp)
{
    auto lptr = left.data.get();
    auto rptr = right.data.get();
    auto dptr = disp.data.get();

    censusTransform(lptr, censusLeft.data(), width, height);
    censusTransform(rptr, censusRight.data(), width, height);
    img::Img<uint32_t> lc(left.width, left.height, (uint32_t*)censusLeft.data());
    img::Img<uint32_t> rc(left.width, left.height, (uint32_t*)censusRight.data());
    img::Img<uint16_t> costI(maxdisp, width, (uint16_t*)costs.data());

//#define RIGHT_FRAME
#ifdef RIGHT_FRAME
    for (int y = B_R; y < height - B_R; y++) {
        costs.assign(width*maxdisp, std::numeric_limits<uint16_t>::max());
        for (int x = B_R; x < width - B_R; x++) {
            auto ul = std::min(width - B_R, x + maxdisp);
            for (int d = x; d < ul; d++) {
                uint16_t cost = 0;
                for (int i = -B_R; i <= B_R; i++) {
                    for (int j = -B_R; j <= B_R; j++) {
                        auto pl = censusLeft[(y + i)*width + (d + j)];
                        auto pr = censusRight[(y + i)*width + (x + j)];

                        cost += popcount(pl ^ pr);
                    }
                }
                costs[x*maxdisp + (d - x)] = cost;
            }
        }
        for (int x = B_R; x < width - B_R; x++) {
            auto minRVal = std::numeric_limits<uint16_t>::max();
            auto minRIdx = 0;
            auto minLVal = std::numeric_limits<uint16_t>::max();
            auto minLIdx = 0;
            for (int d = 0; d < maxdisp; d++){
                auto cost = costs[x*maxdisp + d];
                if (cost < minRVal) {
                    minRVal = cost;
                    minRIdx = d;
                }
            }
            auto xl = std::max(0, x - minRIdx);
            auto xu = std::min(width - 1, xl + maxdisp);
            for (int xd = xl; xd < xu; xd++) {
                auto d = x - xd + minRIdx;
                auto cost = costs[xd*maxdisp + d];
                if (cost < minLVal) {
                    minLVal = cost;
                    minLIdx = d;
                }
            }

            dptr[y*width + x] = abs(minLIdx - minRIdx) < LRT ? minRIdx * muldisp : 0;
        }
    }
#else
    for (int y = B_R; y < height - B_R; y++) {
        costs.assign(width*maxdisp, std::numeric_limits<uint16_t>::max());
        for (int x = B_R; x < width - B_R; x++) {
            auto lb = std::max(B_R, x - maxdisp);
            auto search_limit = x - lb;
            for (int d = 0; d < search_limit; d++) {
                uint16_t cost = 0;
                for (int i = -B_R; i <= B_R; i++) {
                    for (int j = -B_R; j <= B_R; j++) {
                        auto pl = censusLeft[(y + i)*width + (x + j)];
                        auto pr = censusRight[(y + i)*width + (x + j -d)];

                        cost += popcount(pl ^ pr);
                    }
                }
                costs[x*maxdisp + d] = cost;
            }
        }
        for (int x = B_R; x < width - B_R; x++) {
            auto minRVal = std::numeric_limits<uint16_t>::max();
            auto minRIdx = 0;
            auto minLVal = std::numeric_limits<uint16_t>::max();
            auto minLIdx = 0;
            for (int d = 0; d < maxdisp; d++){
                auto cost = costs[x*maxdisp + d];
                if (cost < minLVal) {
                    minLVal = cost;
                    minLIdx = d;
                }
            }
            auto xl = std::max(0, x - minLIdx);
            auto xu = std::min(width - 1, xl + maxdisp);
            for (int xd = xl; xd < xu; xd++) {
                auto d = xd - x + minLIdx;
                auto cost = costs[xd*maxdisp + d];
                if (cost < minRVal) {
                    minRVal = cost;
                    minRIdx = d;
                }
            }
            auto sp = (minLIdx > 0 && minLIdx < maxdisp-1) ? 
                subpixel(costs[x*maxdisp + std::max(minLIdx - 1, 0)],
                                costs[x*maxdisp + minLIdx],
                                costs[x*maxdisp + std::min(minLIdx + 1, maxdisp - 1)]) : 0;
            uint16_t res = (uint16_t)std::round((minLIdx + sp)* muldisp);
            dptr[y*width + x] = abs(minLIdx - minRIdx) < LRT ? res : 0;
        }
    }
#endif

}
