#include "bmMatch.h"
#include <limits>
#include <algorithm>

using namespace stereo;

// Box Filter Radius and Width
#define		B_R		(3)
#define		B_W		(2*B_R+1)

BMatch::BMatch(int w, int h, int d, int m)
	: StereoMatch(w, h, d, m), costs(w*d), edgeLeft(w*h, 0), edgeRight(w*h, 0)
{
}

static void edgeDetect(uint8_t * in, int16_t * out, int w, int h)
{
	for (int y = 1; y < h - 1; y++) {
		for (int x = 1; x < w - 1; x++) {
			auto dx = - in[(y - 1)*w + (x - 1)] 
					  + in[(y - 1)*w + (x + 1)]
					  -2*in[(y - 0)*w + (x - 1)]
					  +2*in[(y - 0)*w + (x + 1)]
					  - in[(y + 1)*w + (x - 1)]
					  + in[(y + 1)*w + (x + 1)];
			auto dy = - in[(y - 1)*w + (x - 1)] 
					  -2*in[(y - 1)*w + (x - 0)]
					  - in[(y - 1)*w + (x + 1)]
					  + in[(y + 1)*w + (x - 1)]
					  +2*in[(y + 1)*w + (x - 0)]
					  + in[(y + 1)*w + (x + 1)];
			out[y*w + x] = (int)std::round(std::sqrtf(dx*dx+dy*dy));
		}
	}
}
static void edgeDetectDummy(uint8_t * in, int16_t * out, int w, int h)
{
	for (int y = 1; y < h - 1; y++) {
		for (int x = 1; x < w - 1; x++) {
			out[y*w + x] = in[y*w + x];
		}
	}
}

static float subpixel(float costLeft, float costMiddle, float costRight)
{
	if (costMiddle >= 0xfffe || costLeft >= 0xfffe || costRight >= 0xfffe)
		return 0.f;
	auto num = costRight - costLeft;
	auto den = (2 * (costRight + costLeft - 2*costMiddle));
	return den != 0 ? 0.5f*( num/ den) : 0;
}
void BMatch::match(img::Img<uint8_t> & left, img::Img<uint8_t> & right, img::Img<uint16_t> & disp)
{
	auto lptr = left.data.get();
	auto rptr = right.data.get();
	auto dptr = disp.data.get();

	edgeDetectDummy(lptr, edgeLeft.data(), width, height);
	edgeDetectDummy(rptr, edgeRight.data(), width, height);

	img::Img<int16_t> lc(left.width, left.height, (int16_t*)edgeLeft.data());
	img::Img<int16_t> rc(left.width, left.height, (int16_t*)edgeRight.data());
	img::Img<float> costI(maxdisp, width, (float*)costs.data());
	auto sqr = [](float diff){ return diff*diff; };

//#define RIGHT_FRAME
#ifdef RIGHT_FRAME
	for (int y = B_R; y < height - B_R; y++) {
		costs.assign(width*maxdisp, std::numeric_limits<float>::max());
		for (int x = B_R; x < width - B_R; x++) {
			auto ul = std::min(width - B_R, x + maxdisp);
			for (int d = x; d < ul; d++) {
				float cost = 0;
				for (int i = -B_R; i <= B_R; i++) {
					for (int j = -B_R; j <= B_R; j++) {
						auto pl = edgeLeft[(y + i)*width + (d + j)];
						auto pr = edgeRight[(y + i)*width + (x + j)];

						cost += sqr(pl - pr);
					}
				}
				costs[x*maxdisp + (d - x)] = cost;
			}
		}
		for (int x = B_R; x < width - B_R; x++) {
			auto minRVal = std::numeric_limits<float>::max();
			auto minRIdx = 0;
			auto minLVal = std::numeric_limits<float>::max();
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

			dptr[y*width + x] = abs(minLIdx - minRIdx) < 2 ? minRIdx * muldisp : 0;
		}
	}
#else
	for (int y = B_R; y < height - B_R; y++) {
		costs.assign(width*maxdisp, std::numeric_limits<float>::max());
		for (int x = B_R; x < width - B_R; x++) {
			auto lb = std::max(B_R, x - maxdisp);
			auto search_limit = x - lb;
			for (int d = 0; d < search_limit; d++) {
				float cost = 0;
				for (int i = -B_R; i <= B_R; i++) {
					for (int j = -B_R; j <= B_R; j++) {
						auto pl = edgeLeft[(y + i)*width + (x + j)];
						auto pr = edgeRight[(y + i)*width + (x + j - d)];

						cost += sqr(pl - pr);
					}
				}
				costs[x*maxdisp + d] = cost;
			}
		}
		for (int x = B_R; x < width - B_R; x++) {
			auto minRVal = std::numeric_limits<float>::max();
			auto minRIdx = 0;
			auto minLVal = std::numeric_limits<float>::max();
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

			auto sp = (minLIdx > 0 && minLIdx < maxdisp - 1) ?
				subpixel(costs[x*maxdisp + std::max(minLIdx - 1, 0)],
				costs[x*maxdisp + minLIdx],
				costs[x*maxdisp + std::min(minLIdx + 1, maxdisp - 1)]) : 0;
			auto res = std::round((minLIdx + sp)* muldisp);
			dptr[y*width + x] = abs(minLIdx - minRIdx) < 2 ? res : 0;
		}
	}
#endif

}
