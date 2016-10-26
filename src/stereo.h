#pragma once

#include "image.h"
#include <cstring> // memset
#include <vector>
#include <string>
#include <fstream>
namespace stereo {

class StereoMatch {
public:
    StereoMatch(int w, int h, int d, int m)
        : width(w)
        , height(h)
        , maxdisp(d)
        , muldisp(m){};
    virtual void match(img::Img<uint16_t>& left, img::Img<uint16_t>& right,
        img::Img<uint16_t>& disp, img::Img<uint8_t>& conf)
        = 0;
    std::pair<img::Img<uint16_t>, img::Img<uint8_t>> match(img::Img<uint16_t>& left, img::Img<uint16_t>& right)
    {
        img::Img<uint16_t> disp(left.width, left.height,uint16_t(0));
		img::Img<uint8_t> conf(left.width, left.height,uint8_t(0));

        this->match(left, right, disp, conf);
        return std::make_pair(disp,conf);
    }

public: // but please don't set!
    std::string costsName; 
    int width, height, maxdisp, muldisp;
};
}
