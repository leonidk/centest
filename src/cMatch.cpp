#include "cMatch.h"
#include <limits>
#include <algorithm>

using namespace stereo;

// Census Radius and Width
#define		C_R		(3)
#define		C_W		(2*C_R+1)

// Box Filter Radius and Width
#define		B_R		(3)
#define		B_W		(2*B_R+1)

// y,x
const int samples[] =
{ 
	-3,-2,
    -3, 0,
    -3, 2,
	-2,-3,
    -2,-1,
    -2, 1,
    -2, 3,
	-1,-2,
    -1, 0,
    -1, 2,
	 0,-3,
     0,-1,
     0, 1,
     0, 3,
	 1,-2,
     1, 0,
     1, 2,
	 2,-3,
     2,-1,
     2, 1,
     2, 3,
	 3,-2,
     3, 0,
     3, 2
};


CensusMatch::CensusMatch(int w, int h, int d, int m)
	: StereoMatch(w, h, d, m), costs(w*d), censusLeft(w*h,0), censusRight(w*h,0)
{
}

static void censusTransform(uint8_t * in, uint32_t * out, int w, int h)
{
    int ns = (int)(sizeof(samples)/sizeof(int))/2;
    for(int y=C_R; y < h - C_R; y++) {
        for(int x=C_R; x < w - C_R; x++) {
			uint32_t px = 0;
			auto center = in[y*w + x];
            for(int p=0; p < ns; p++) {
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

void CensusMatch::match(img::Img<uint8_t> & left, img::Img<uint8_t> & right, img::Img<uint16_t> & disp)
{
    auto lptr = left.data.get();
    auto rptr = right.data.get();
    auto dptr = disp.data.get();

    censusTransform(lptr,censusLeft.data(),width,height);
    censusTransform(rptr,censusRight.data(),width,height);
	img::Img<uint32_t> lc(left.width, left.height, (uint32_t*)censusLeft.data());
	img::Img<uint32_t> rc(left.width, left.height, (uint32_t*)censusRight.data());
	img::Img<uint16_t> costI(maxdisp, width, (uint16_t*)costs.data());

    for(int y=B_R; y < height - B_R; y++) {
        costs.assign(width*maxdisp,std::numeric_limits<uint16_t>::max());
        for(int x=B_R; x < width - B_R; x++) {
            //auto bl = std::max(0,x-maxdisp+1);
			auto ul = std::min(width - B_R, x + maxdisp);
            for(int d=x; d < ul; d++) {
				uint16_t cost = 0;
				for (int i = -B_R; i <= B_R; i++) {
					for (int j = -B_R; j <= B_R; j++) {
						auto pl = censusLeft[(y + i)*width + (x + j)];
						auto pr = censusRight[(y + i)*width + (d + j)];

						cost += popcount(pl ^ pr);
					}
				}
				costs[x*maxdisp + (d-x)] = cost; 
            }
        }
        for(int x=B_R; x < width - B_R; x++) {
			auto minVal = std::numeric_limits<uint16_t>::max();
			auto minIdx = 0;
			for (int d = 0; d < maxdisp; d++){
				auto cost = costs[x*maxdisp + d];
				if (cost < minVal) {
					minVal = cost;
					minIdx = d;
				}
			}
            //uint16_t minRL = 0;
            //for(
            //printf("c: %d %ld %d\n",x, dis, *min_elem);
			dptr[y*width + x] = minIdx * muldisp;
        }
    }
}
