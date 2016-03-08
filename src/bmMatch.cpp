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


void BMatch::match(img::Img<uint8_t> & left, img::Img<uint8_t> & right, img::Img<uint16_t> & disp)
{
	auto lptr = left.data.get();
	auto rptr = right.data.get();
	auto dptr = disp.data.get();

//	//censusTransform(lptr, censusLeft.data(), width, height);
//	//censusTransform(rptr, censusRight.data(), width, height);
//	img::Img<uint32_t> lc(left.width, left.height, (uint32_t*)censusLeft.data());
//	img::Img<uint32_t> rc(left.width, left.height, (uint32_t*)censusRight.data());
//	img::Img<uint16_t> costI(maxdisp, width, (uint16_t*)costs.data());
//
//	//#define RIGHT_FRAME
//#ifdef RIGHT_FRAME
//	for (int y = B_R; y < height - B_R; y++) {
//		costs.assign(width*maxdisp, std::numeric_limits<uint16_t>::max());
//		for (int x = B_R; x < width - B_R; x++) {
//			auto ul = std::min(width - B_R, x + maxdisp);
//			for (int d = x; d < ul; d++) {
//				uint16_t cost = 0;
//				for (int i = -B_R; i <= B_R; i++) {
//					for (int j = -B_R; j <= B_R; j++) {
//						auto pl = censusLeft[(y + i)*width + (d + j)];
//						auto pr = censusRight[(y + i)*width + (x + j)];
//
//						cost += popcount(pl ^ pr);
//					}
//				}
//				costs[x*maxdisp + (d - x)] = cost;
//			}
//		}
//		for (int x = B_R; x < width - B_R; x++) {
//			auto minRVal = std::numeric_limits<uint16_t>::max();
//			auto minRIdx = 0;
//			auto minLVal = std::numeric_limits<uint16_t>::max();
//			auto minLIdx = 0;
//			for (int d = 0; d < maxdisp; d++){
//				auto cost = costs[x*maxdisp + d];
//				if (cost < minRVal) {
//					minRVal = cost;
//					minRIdx = d;
//				}
//			}
//			auto xl = std::max(0, x - minRIdx);
//			auto xu = std::min(width - 1, xl + maxdisp);
//			for (int xd = xl; xd < xu; xd++) {
//				auto d = x - xd + minRIdx;
//				auto cost = costs[xd*maxdisp + d];
//				if (cost < minLVal) {
//					minLVal = cost;
//					minLIdx = d;
//				}
//			}
//
//			dptr[y*width + x] = abs(minLIdx - minRIdx) < 2 ? minRIdx * muldisp : 0;
//		}
//	}
//#else
//	for (int y = B_R; y < height - B_R; y++) {
//		costs.assign(width*maxdisp, std::numeric_limits<uint16_t>::max());
//		for (int x = B_R; x < width - B_R; x++) {
//			auto lb = std::max(B_R, x - maxdisp);
//			auto search_limit = x - lb;
//			for (int d = 0; d < search_limit; d++) {
//				uint16_t cost = 0;
//				for (int i = -B_R; i <= B_R; i++) {
//					for (int j = -B_R; j <= B_R; j++) {
//						auto pl = censusLeft[(y + i)*width + (x + j)];
//						auto pr = censusRight[(y + i)*width + (x + j - d)];
//
//						cost += popcount(pl ^ pr);
//					}
//				}
//				costs[x*maxdisp + d] = cost;
//			}
//		}
//		for (int x = B_R; x < width - B_R; x++) {
//			auto minRVal = std::numeric_limits<uint16_t>::max();
//			auto minRIdx = 0;
//			auto minLVal = std::numeric_limits<uint16_t>::max();
//			auto minLIdx = 0;
//			for (int d = 0; d < maxdisp; d++){
//				auto cost = costs[x*maxdisp + d];
//				if (cost < minLVal) {
//					minLVal = cost;
//					minLIdx = d;
//				}
//			}
//			auto xl = std::max(0, x - minLIdx);
//			auto xu = std::min(width - 1, xl + maxdisp);
//			for (int xd = xl; xd < xu; xd++) {
//				auto d = xd - x + minLIdx;
//				auto cost = costs[xd*maxdisp + d];
//				if (cost < minRVal) {
//					minRVal = cost;
//					minRIdx = d;
//				}
//			}
//
//			dptr[y*width + x] = abs(minLIdx - minRIdx) < 2 ? minLIdx * muldisp : 0;
//		}
//	}
//#endif

}
