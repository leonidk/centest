#pragma once
#include <memory>
namespace img {
	
	template <typename T, int C>
	struct Image {
		std::shared_ptr<T> data;
		int width, height;
		Image() : data(nullptr),width(0),height(0) {}
		Image(int width, int height) : data(new T[width*height*C], arr_d()),width(width),height(height) {}
		Image(int width, int height, T* d) : data(d, null_d()), width(width), height(height)  {}

		struct null_d { void operator ()(T const * p)	{ } };
		struct arr_d { void operator ()(T const * p)	{ delete[] p; } };
	};

	template<typename T> using Img = Image<T, 1>;
}
