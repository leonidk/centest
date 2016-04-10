#pragma once

#include "stereo.h"
namespace stereo {

    class CensusMatch : public StereoMatch {
    public:
        using StereoMatch::match;

        CensusMatch(int w, int h, int d, int m);
        virtual void match(img::Img<uint8_t> & left, img::Img<uint8_t> & right,
            img::Img<uint16_t> & disp) override;
    private:
        std::vector<uint32_t> censusLeft,censusRight;
        std::vector<uint16_t> costs;
    };
}
