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
    auto ns = (sizeof(samples)/sizeof(int))/2;
    for(int y=C_R; y < h - C_R; y++) {
        for(int x=C_R; x < w - C_R; x++) {
            for(int p=0; p < ns; p++) {
                out[y*w+x] &= (in[(y+samples[2*p])*w+(x+samples[2*p+1])])<<p;
            }
        }
    }
}

void CensusMatch::match(img::Img<uint8_t> & left, img::Img<uint8_t> & right, img::Img<uint16_t> & disp)
{
    auto lptr = left.data.get();
    auto rptr = right.data.get();
    auto dptr = disp.data.get();

    censusTransform(lptr,censusLeft.data(),width,height);
    censusTransform(rptr,censusRight.data(),width,height);
    
    for(int y=B_R; y < height - B_R; y++) {
        costs.assign(width*maxdisp,std::numeric_limits<uint16_t>::max());
        for(int x=B_R; x < width - B_R; x++) {
            auto pl = censusLeft[y*width + x];
            auto bl = std::max(0,x-maxdisp);
            for(int d=bl; d < x; d++) {
                //printf("%d %d\n",x,d);
                auto pr = censusRight[y*width + d];
                costs[x*maxdisp+(x-d)] = __builtin_popcount(pl ^ pr);
            }
        }
        for(int x=B_R; x < width - B_R; x++) {
            auto cstart = costs.begin() + x*maxdisp;
            auto min_elem = std::min_element(cstart,cstart + maxdisp);
            auto dis = std::distance(cstart,min_elem);
            printf("c: %d %ld\n",x, dis);
            dptr[y*width+x] = dis * muldisp; 
        }
    }
}
