#pragma once
#include <algorithm>
#include <cmath>
#include <memory>
#include <cstring>
static float clamp_f(float min, float max, float x)
{
    return std::max(min, std::min(max, x));
}

namespace img {

    template <typename T, int C>
    struct Image {
        std::shared_ptr<T> data;
        int width, height;
        T * ptr;
        Image()
            : data(nullptr)
            , width(0)
            , height(0)
            , ptr(nullptr)
        {
        }
        Image(int width, int height)
            : data(new T[width * height * C], arr_d())
            , width(width)
            , height(height)
        {
            ptr = data.get();
        }
        Image(int width, int height, T d)
            : Image(width, height)
        {
            std::fill(ptr, ptr + width*height*C, d);
        }
        Image(int width, int height, T* d)
            : data(d, null_d())
            , width(width)
            , height(height)
            , ptr(d)
        {
        }

        struct null_d {
            void operator()(T const* p) {}
        };
        struct arr_d {
            void operator()(T const* p) { delete[] p; }
        };
        T& operator()(int i) { return ptr[i]; }
        T& operator()(int y, int x) { return ptr[y*width + x]; }
        T& operator()(int y, int x, int c) { return ptr[C*(y*width + x) + c]; }


        inline T sample(const float x, const float y, const int chan)
        {
            auto pixX = [this](float x) { return (int)clamp_f(0.0f, (float)(width - 1), std::round(x)); };
            auto pixY = [this](float y) { return (int)clamp_f(0.0f, (float)(height - 1), std::round(y)); };

            auto xm = pixX(x - 0.5f);
            auto xp = pixX(x + 0.5f);
            auto ym = pixY(y - 0.5f);
            auto yp = pixY(y + 0.5f);
            auto ptr = data.get();

            auto tl = ptr[C * (ym * width + xm) + chan];
            auto tr = ptr[C * (ym * width + xp) + chan];
            auto bl = ptr[C * (yp * width + xm) + chan];
            auto br = ptr[C * (yp * width + xp) + chan];

            float dx = x - xm;
            float dy = y - ym;

            auto sample = tl * (1.f - dx) * (1.f - dy) + tr * dx * (1.f - dy) + bl * (1.f - dx) * dy + br * dx * dy;
            return (T)sample;
        }
        img::Image<T, C> copy()
        {
            img::Image<T, C> res(width, height);
            memcpy(res.data.get(), this->data.get(), width*height*sizeof(T)*C);
            res.ptr = res.data.get();
            return res;
        }
    };

    template <typename T>
    using Img = Image<T, 1>;
}