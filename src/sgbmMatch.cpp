#include "sgbmMatch.h"
#include <algorithm>
#include <limits>
#include <fstream>
#include <cmath>
using namespace stereo;

// Using the parameters from
// https://github.com/IntelRealSense/librealsense/blob/master/include/librealsense/rsutil.h

// Census Radius and Width
#define C_R (3)
#define C_W (2 * C_R + 1)

// Box Filter Radius and Width
#define B_R (3)
#define B_W (2 * B_R + 1)

// Cost Multipliers
#define C_M (3)
#define A_M (1)

// Left-Right Threshold
// Dependent on subpixel algorithm. May not work well with subpixel
#define LRT (2)
#define LRS (0.75f)

// Neighbor Threshold
#define NT (20)

// Second Peak
#define SP (50)

// Texture Diff (commented out below)
#define TD (4)
#define TC (6)

// Score Limits
#define SMIN (1)
#define SMAX (20000)

// Median
#define MP (10)
#define MM (10)
#define MT (500)

// SGM Settings
#define P1 (2000)
#define P2 (4 * P1)

#define MAXCOST (SMAX) //(0x00ffffff)
#define USE_SGM 1

// scale SGM P2 as P2 = P2/grad.
// should make propogation stop at edges
#define SCALE_P2 1

//bilateral filter on the weights
#define USE_BLF 0
#define RANGESIGMA (5 * 5)
#define SPACESIGMA ((B_R / 2.0f) * (B_R / 2.0f))

// hole filling
#define MOVE_LEFT 0

const int output_log = 1;
const std::string input_file("1dcnn-nin-moto.txt");
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


sgbmMatch::sgbmMatch(int w, int h, int d, int m)
    : StereoMatch(w, h, d, m)
    , costs(w * d)
    , costsSummed(w * d)
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
static float subpixel(float costLeft, float costMiddle, float costRight)
{
    if (costMiddle >= 0xfffe || costLeft >= 0xfffe || costRight >= 0xfffe)
        return 0.f;

    auto num = costRight - costLeft;
    auto den = (costRight < costLeft) ? (costMiddle - costLeft) : (costMiddle - costRight);
    return den != 0 ? 0.5f * (num / den) : 0;
}

void sgbmMatch::match(img::Img<uint8_t>& left, img::Img<uint8_t>& right, img::Img<uint16_t>& disp)
{
    img::Img<float> gt;
    this->match(left, right, gt, disp);
}
void sgbmMatch::match(img::Img<uint8_t>& left, img::Img<uint8_t>& right, img::Img<float> & gt, img::Img<uint16_t>& disp)
{
    auto lptr = left.data.get();
    auto rptr = right.data.get();
    auto dptr = disp.data.get();
    auto gptr = gt.data.get();

    censusTransform(lptr, censusLeft.data(), width, height);
    censusTransform(rptr, censusRight.data(), width, height);
    img::Img<uint32_t> lc(left.width, left.height, (uint32_t*)censusLeft.data());
    img::Img<uint32_t> rc(left.width, left.height, (uint32_t*)censusRight.data());
    img::Img<uint32_t> costI(maxdisp, width, (uint32_t*)costs.data());
    costsSummed.assign(width * maxdisp, MAXCOST);

    std::vector<int32_t> topCosts(width * maxdisp, MAXCOST);
    std::vector<int32_t> topLeftCosts(width * maxdisp, MAXCOST);
    std::vector<int32_t> topRightCosts(width * maxdisp, MAXCOST);
    float bilateralWeights[B_W * B_W];

    std::ofstream gtOut; if (output_log) gtOut = std::ofstream("gt.csv");
    std::ofstream sgbmOut; if (output_log) sgbmOut = std::ofstream("sgbm.csv");
    std::ofstream rawOut; if (output_log) rawOut = std::ofstream("raw.csv");

    std::ifstream predIn; if (input_file.size()) predIn = std::ifstream(input_file);
    
    for (int y = B_R; y < height - B_R; y++) {
        printf("\r %.2lf %%", 100.0*static_cast<double>(y) / static_cast<double>(height));
        auto prevVal = 0;
        costs.assign(width * maxdisp, MAXCOST);
        if (USE_BLF) {
             #pragma omp parallel for
            for (int x = B_R; x < width - B_R; x++) {
                auto lb = std::max(B_R, x - maxdisp);
                auto search_limit = x - lb;
                float middlePx = lptr[(y)*width + (x)];
                float blW = 0;
                for (int i = -B_R; i <= B_R; i++) {
                    for (int j = -B_R; j <= B_R; j++) {
                        float px = lptr[(y + i) * width + (x + j)];
                        auto rw = expf(-(px - middlePx) / (2.f * RANGESIGMA));
                        auto sw = expf(-sqrtf((float)(i * i + j * j)) / (2.f * SPACESIGMA));
                        bilateralWeights[(i + B_R) * B_W + (j + B_R)] = rw * sw;
                        blW += rw * sw;
                    }
                }

                for (int d = 0; d < search_limit; d++) {
                    float cost = 0;
                    for (int i = -B_R; i <= B_R; i++) {
                        for (int j = -B_R; j <= B_R; j++) {
                            auto pl = censusLeft[(y + i) * width + (x + j)];
                            auto pr = censusRight[(y + i) * width + (x + j - d)];

                            auto al = lptr[(y + i) * width + (x + j)];
                            auto ar = rptr[(y + i) * width + (x + j - d)];

                            cost += bilateralWeights[(i + B_R) * B_W + (j + B_R)] * (C_M * popcount(pl ^ pr) + A_M * abs(al - ar));
                        }
                    }
                    costs[x * maxdisp + d] = (uint32_t)std::round((B_W * B_W) * cost / blW);
                }
            }
        } else {
             #pragma omp parallel for
            for (int x = B_R; x < width - B_R; x++) {
                auto lb = std::max(B_R, x - maxdisp);
                auto search_limit = x - lb;
                for (int d = 0; d < search_limit; d++) {
                    uint16_t cost = 0;
                    for (int i = -B_R; i <= B_R; i++) {
                        for (int j = -B_R; j <= B_R; j++) {
                            auto pl = censusLeft[(y + i) * width + (x + j)];
                            auto pr = censusRight[(y + i) * width + (x + j - d)];

                            auto al = lptr[(y + i) * width + (x + j)];
                            auto ar = rptr[(y + i) * width + (x + j - d)];

                            cost += C_M * popcount(pl ^ pr) + A_M * abs(al - ar);
                        }
                    }
                    costs[x * maxdisp + d] = cost;
                }
            }
        }
        auto clipD = [&](int d) { return std::min(maxdisp - 1, std::max(0, d)); };

        //  semiglobal
        if (USE_SGM) {
            std::vector<int32_t> leftCosts(width * maxdisp, MAXCOST);
            std::vector<int32_t> rightCosts(width * maxdisp, MAXCOST);

            //left
            for (int x = 1; x < width; x++) {
                auto p1 = P1;
                auto p2 = P2;
                if (SCALE_P2) {
                    auto grad = abs(lptr[y * width + x - 1] - lptr[y * width + x]) + 1;
                    p2 = (p2 + grad - 1) / grad;
                    p1 = std::min(p1, p2 - 1);
                }
                auto lftI = 0;
                auto lftV = MAXCOST;
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
                auto p1 = P1;
                auto p2 = P2;
                if (SCALE_P2) {
                    auto grad = abs(lptr[(y - 1) * width + x] - lptr[y * width + x]) + 1;
                    p2 = (p2 + grad - 1) / grad;
                    p1 = std::min(p1, p2 - 1);
                }
                auto topI = 0;
                auto topV = MAXCOST;
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
                auto p1 = P1;
                auto p2 = P2;
                if (SCALE_P2) {
                    auto grad = abs(lptr[(y - 1) * width + x - 1] - lptr[y * width + x]) + 1;
                    p2 = (p2 + grad - 1) / grad;
                    p1 = std::min(p1, p2 - 1);
                }
                auto tplI = 0;
                auto tplV = MAXCOST;
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
                auto p1 = P1;
                auto p2 = P2;
                if (SCALE_P2) {
                    auto grad = abs(lptr[(y)*width + x + 1] - lptr[y * width + x]) + 1;
                    p2 = (p2 + grad - 1) / grad;
                    p1 = std::min(p1, p2 - 1);
                }
                auto rgtI = 0;
                auto rgtV = MAXCOST;
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
                auto p1 = P1;
                auto p2 = P2;
                if (SCALE_P2) {
                    auto grad = abs(lptr[(y - 1) * width + x + 1] - lptr[y * width + x]) + 1;
                    p2 = (p2 + grad - 1) / grad;
                    p1 = std::min(p1, p2 - 1);
                }
                auto rgtI = 0;
                auto rgtV = MAXCOST;
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
        //min selection
        for (int x = B_R; x < width - B_R; x++) {
            auto minRVal = (uint32_t)MAXCOST;
            auto minRIdx = 0;
            auto minLVal = (uint32_t)MAXCOST;
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
            auto spL = (minLIdx > 0 && minLIdx < maxdisp - 1) ? subpixel((float)nL, (float)nC, (float)nR) : 0;
            // subpixel right
            auto rL = costsSummed[(x - 1) * maxdisp + std::max(minLIdx - 1, 0)];
            auto rC = costsSummed[(x)*maxdisp + minLIdx];
            auto rR = costsSummed[(x + 1) * maxdisp + std::min(minLIdx + 1, maxdisp - 1)];
            auto spR = (minLIdx > 0 && minLIdx < maxdisp - 1) ? subpixel((float)rL, (float)rC, (float)rR) : 0;

            // disparity computation
            uint16_t res = (uint16_t)std::round((minLIdx + spL) * muldisp);
            if (input_file.size()) {
                auto correct = gt.ptr[y*width + x];
                if (std::isfinite(correct)) {
                    float predDisp;
                    predIn >> predDisp;
                    int intDisp = (int)predDisp;
                    res = (uint16_t)(muldisp*predDisp);
                }
                else {
                    res = 0;
                }
            }
            else {
                auto correct = gt.ptr[y*width + x];
                if (!std::isfinite(correct)) {
                    res = 0;
                }
            }

            // left-right threshold
            res = abs(minLIdx - minRIdx) < LRT && abs(spR - spL) < LRS ? res : 0;

            // neighbor threshold
            auto diffL = (int)nL - (int)nC;
            auto diffR = (int)nR - (int)nC;
            res = (diffL > NT || diffR > NT) ? res : 0;

            // second peak threshold
            uint32_t minL2Val = MAXCOST;
            for (int d = 0; d < maxdisp; d++) {
                auto cost = costsSummed[x * maxdisp + d];
                if (d == minLIdx || d == minLIdx + 1 || d == minLIdx - 1)
                    continue;
                if (cost < minL2Val)
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
            uint32_t me = MAXCOST;
            auto initialized = false;
            for (int d = 0; d < maxdisp; d++) {
                auto cost = costsSummed[x * maxdisp + d];
                if (!initialized && cost != MAXCOST) {
                    initialized = true;
                    me = cost;
                }
                if (cost > me)
                    me += MP;
                else if (cost < me)
                    me -= MM;
            }
            res = (me - minLVal > MT) ? res : 0;

            // hole filling
            if (MOVE_LEFT) {
                prevVal = res ? res : prevVal;
                res = res ? res : prevVal;
            }
            // final set
            dptr[y * width + x] = res;

            if (output_log) {
                if (std::isfinite(gt.ptr[y*width + x])) {
                    auto gtInt = (int)std::round(gt.ptr[y*width + x]);
                    gtOut << gtInt << '\n';
                    for (int i = 0; i < this->maxdisp; i++) {
                        rawOut << costs[x*maxdisp + i];
                        if (i != maxdisp - 1)
                            rawOut << ',';
                    }
                    rawOut << '\n';
                    for (int i = 0; i < this->maxdisp; i++) {
                        sgbmOut << costsSummed[x*maxdisp + i];
                        if (i != maxdisp - 1)
                            sgbmOut << ',';
                    }
                    sgbmOut << '\n';
                }
            }
        }
    }
}
