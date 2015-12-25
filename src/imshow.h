#pragma once
#include "image.h"
#include <algorithm>
namespace img {
	//imshow.cpp
	template <typename T, int C>
	void imshow(const char * name, const Image<T, C> &img);
	char getKey(bool wait = false);

	template <typename T, int C>
	void convertToGrey(Image<T, C> & img_in, Image<uint8_t, C> & img_out, T min_val, T max_val)
	{
		auto cnt = img_in.width*img_in.height*C;
		auto scale = (255.0 / (double)(max_val-min_val));
		auto in_img = img_in.ptr();
		auto out_img = img_out.ptr();
		
		for (int i = 0; i < cnt; i++) {
			auto v = std::min<T>(std::max<T>(in_img[i],min_val),max_val);
			out_img[i] = (uint8_t)(scale*(v-min_val));
		}
	}
	template <typename T, int C>
	Image<uint8_t, C> convertToGrey(Image<T, C> & img_in, T min_val, T max_val) {
		Image<uint8_t, C> output(img_in.width, img_in.height);
		convertToGrey(img_in, output, min_val, max_val);
		return output;
	}
}