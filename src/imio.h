#pragma once
#include "image.h"

namespace img {
//imio.cpp
template <typename T, int C>
Image<T, C> imread(const char* name);

template <typename T, int C>
void imwrite(const char* name, const Image<T, C>& img);

//imshow.cpp
template <typename T, int C>
void imshow(const char* name, const Image<T, C>& img);
char getKey(bool wait = false);
}