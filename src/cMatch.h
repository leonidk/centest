#pragma once

#include "image.h"

namespace stereo {

	class StereoMatch {
	public:
		StereoMatch(int w, int h, int d) : width(w), height(h), maxdisp(d) {};
		virtual void match(img::Img<uint8_t> & left,img::Img<uint8_t> & right,
						   img::Img<uint16_t> & disp) = 0;
		img::Img<uint16_t> match(img::Img<uint8_t> & left, img::Img<uint8_t> & right)
		{
			img::Img<uint16_t> disp(left.width, left.height);
			this->match(left, right, disp);
			return disp;
		}
	protected:
		int width, height, maxdisp;
	};

	class CensusMatch : public StereoMatch {
	public:
		using StereoMatch::match;

		CensusMatch(int w, int h, int d);
		virtual void match(img::Img<uint8_t> & left, img::Img<uint8_t> & right,
			img::Img<uint16_t> & disp) override;
	};
}