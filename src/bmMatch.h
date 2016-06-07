#pragma once

#include "stereo.h"
namespace stereo {

class BMatch : public StereoMatch {
public:
    using StereoMatch::match;

    BMatch(int w, int h, int d, int m);
    virtual void match(img::Img<uint8_t>& left, img::Img<uint8_t>& right,
        img::Img<uint16_t>& disp) override;

private:
    std::vector<int16_t> edgeLeft, edgeRight;
    std::vector<float> costs;
};
}
