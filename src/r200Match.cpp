#include "R200Match.h"
#include <limits>
#include <algorithm>

using namespace stereo;

// Using the parameters from 
// https://github.com/IntelRealSense/librealsense/blob/master/include/librealsense/rsutil.h

// Census Radius and Width
#define        C_R      (3)
#define        C_W      (2*C_R+1)

// Box Filter Radius and Width
#define        B_R      (3)
#define        B_W      (2*B_R+1)

// Left-Right Threshold
// Dependent on subpixel algorithm. May not work well with subpixel
#define        LRT      (2)
#define        LRS      (0.75f)

// Neighbor Threshold
#define        NT       (7)

// Second Peak
#define        SP       (10)

// Texture Diff (commented out below)
#define        TD       (4)
#define        TC       (6)

// Score Limits
#define        SMIN     (1)
#define        SMAX     (384)

// Median
#define        MP       (5)
#define        MM       (5)
#define        MT       (192)

// sampling pattern
// . X . X . X .
// X . X . X . X
// . X . X . X .
// X . X 0 X . X
// . X . X . X .
// X . X . X . X
// . X . X . X .

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


R200Match::R200Match(int w, int h, int d, int m)
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

void R200Match::match(img::Img<uint8_t> & left, img::Img<uint8_t> & right, img::Img<uint16_t> & disp)
{
    auto lptr = left.data.get();
    auto rptr = right.data.get();
    auto dptr = disp.data.get();

    censusTransform(lptr, censusLeft.data(), width, height);
    censusTransform(rptr, censusRight.data(), width, height);
    img::Img<uint32_t> lc(left.width, left.height, (uint32_t*)censusLeft.data());
    img::Img<uint32_t> rc(left.width, left.height, (uint32_t*)censusRight.data());
    img::Img<uint16_t> costI(maxdisp, width, (uint16_t*)costs.data());

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
            // subpixel left
            auto nL = costs[x*maxdisp + std::max(minLIdx - 1, 0)];
            auto nC = costs[x*maxdisp + minLIdx];
            auto nR = costs[x*maxdisp + std::min(minLIdx + 1, maxdisp - 1)];
            auto spL = (minLIdx > 0 && minLIdx < maxdisp-1) ? 
                subpixel(nL,nC,nR) : 0;
            // subpixel right
            auto rL = costs[(x-1)*maxdisp + minLIdx-1];
            auto rC = costs[(x)*maxdisp + minLIdx];
            auto rR = costs[(x+1)*maxdisp + minLIdx+1];
            auto spR = (minLIdx > 0 && minLIdx < maxdisp - 1) ?
                subpixel(rL,rC,rR) : 0;

            // disparity computation
            uint16_t res = (uint16_t)std::round((minLIdx + spL)* muldisp);

            // left-right threshold
            res = abs(minLIdx - minRIdx) <  LRT && abs(spR - spL) < LRS ? res : 0;

            // neighbor threshold
            auto diffL = (int)nL - (int)nC;
            auto diffR = (int)nR - (int)nC;
            res = (diffL > NT || diffR > NT) ? res : 0;

            // second peak threshold
            auto minL2Val = std::numeric_limits<uint16_t>::max();
            for (int d = 0; d < maxdisp; d++){
                auto cost = costs[x*maxdisp + d];
                if (d==minLIdx || d == minLIdx+1 || d==minLIdx-1)
                    continue;
                if(cost < minL2Val)
                    minL2Val = cost;
            }
            auto diffSP = minL2Val - minLVal;
            res = (diffSP > SP) ? res : 0;

            // texture difference (waste of time?)
            //auto tc = 0;
            //int centerV = lptr[y*width + x];
            //for (int i = -B_R; i <= B_R; i++) {
            //    for (int j = -B_R; j <= B_R; j++) {
            //        int v = lptr[(y + i)*width + (x + j)];
            //        tc += abs(centerV - v) > TD ? 1 : 0;
            //    }
            //}
            //res = (tc >= TC) ? res : 0;

            // score limits
            res = (minLVal >= SMIN && minLVal <= SMAX) ? res : 0;

            // median threshold
            auto me = std::numeric_limits<uint16_t>::max();
            auto initialized = false;
            for (int d = 0; d < maxdisp; d++){
                auto cost = costs[x*maxdisp + d];
                if (!initialized && cost != std::numeric_limits<uint16_t>::max()) {
                    initialized = true;
                    me = cost;
                }
                if (cost > me)
                    me += MP;
                else if (cost < me)
                    me -= MM;
            }
            res = (me-minLVal > MT) ? res : 0;

            // final set
            dptr[y*width + x] = res;
        }
    }
}
