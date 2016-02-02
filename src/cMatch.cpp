#include "cMatch.h"
#include <limits>

using namespace stereo;


CensusMatch::CensusMatch(int w, int h, int d, int m)
	: StereoMatch(w, h, d, m), costs(w*d)
{

}

void CensusMatch::match(img::Img<uint8_t> & left, img::Img<uint8_t> & right, img::Img<uint16_t> & disp)
{
    costs.assign(width*maxdisp,std::numeric_limits<uint16_t>::max());
}
