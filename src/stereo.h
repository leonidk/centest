#pragma once

#include "image.h"
#include <cstring> // memset
#include <vector>
namespace stereo {

class StereoMatch {
public:
    StereoMatch(int w, int h, int d, int m)
        : width(w)
        , height(h)
        , maxdisp(d)
        , muldisp(m){};
    virtual void match(img::Img<uint8_t>& left, img::Img<uint8_t>& right,
        img::Img<uint16_t>& disp)
        = 0;
    img::Img<uint16_t> match(img::Img<uint8_t>& left, img::Img<uint8_t>& right)
    {
        img::Img<uint16_t> disp(left.width, left.height);
        memset(disp.data.get(), 0, width * height * sizeof(uint16_t));
        this->match(left, right, disp);
        return disp;
    }

public: // but please don't set!
    int width, height, maxdisp, muldisp;
};
}
