#pragma once
#include "image.h"

namespace img {
namespace detail {
    template <typename T, typename TT>
    Image<T, 3> _grey2Rgb(const Image<T, 1>& input)
    {
        Image<T, 3> output(input.width, input.height);
        T* in_data = (T*)input.data.get();
        T* ot_data = (T*)output.data.get();
        auto stride = input.width * 3;
        for (int y = 0; y < input.height; y++) {
            for (int x = 0; x < input.width; x++) {
                for (int c = 0; c < 3; c++) {
                    ot_data[y * stride + x * 3 + c] = in_data[y * input.width + x];
                }
            }
        }
        return output;
    }

    template <typename T, typename TT>
    Image<T, 1> _Rgb2grey(const Image<T, 3>& input)
    {
        Image<T, 1> output(input.width, input.height);
        T* in_data = (T*)input.data.get();
        T* ot_data = (T*)output.data.get();
        auto stride = input.width * 3;
        for (int y = 0; y < input.height; y++) {
            for (int x = 0; x < input.width; x++) {
                TT sum = 0;
                for (int c = 0; c < 3; c++) {
                    sum += in_data[y * stride + x * 3 + c];
                }
                ot_data[y * input.width + x] = (sum + 2) / 3;
            }
        }
        return output;
    }

    template <typename T, int C, typename TT>
    Image<TT, C> _intImageInc(const Image<T, C>& input)
    {
        Image<TT, C> output(input.width, input.height);

        T* in_data = (T*)input.data.get();
        TT* ot_data = (TT*)output.data.get();
        for (int x = 0; x < input.width * C; x++) {
            ot_data[x] = in_data[x];
        }
        for (int y = 1; y < input.height; y++) {
            TT sum[C] = {};
            for (int x = 0; x < input.width; x++) {
                for (int c = 0; c < C; c++) {
                    sum[c] += in_data[(y - 1) * input.width * C + x * C + c];
                    ot_data[y * input.width * C + x * C + c] = sum[c] + ot_data[(y - 1) * input.width * C + x * C + c];
                }
            }
        }
        return output;
    }
    template <typename T, int C, typename TT>
    Image<TT, C> _intImageEx(const Image<T, C>& input)
    {
        Image<TT, C> output(input.width + 1, input.height + 1);

        T* in_data = (T*)input.data.get();
        TT* ot_data = (TT*)output.data.get();
        auto stride = C * (input.width + 1);
        for (int x = 0; x < stride; x++) {
            ot_data[x] = 0;
        }
        for (int y = 1; y < input.height + 1; y++) {
            ot_data[y * stride] = 0;
            TT sum[C] = {};
            for (int x = 0; x < input.width; x++) {
                for (int c = 0; c < C; c++) {
                    sum[c] += in_data[(y - 1) * input.width * C + x * C + c];
                    ot_data[y * stride + (x + 1) * C + c] = sum[c] + ot_data[(y - 1) * stride + (x + 1) * C + c];
                }
            }
        }
        return output;
    }
    template <typename T, int C, typename TT, int k_w>
    Image<T, C> _boxFilter(const Image<T, C>& input)
    {
        Image<T, C> output(input.width, input.height);
        T* in_data = (T*)input.data.get();
        T* ot_data = (T*)output.data.get();
        memcpy(ot_data, in_data, sizeof(T) * input.width * input.height * C);
        auto stride = input.width * C;
        auto oset = k_w / 2;
        for (int y = oset; y < input.height - oset; y++) {
            for (int x = oset; x < input.width - oset; x++) {
                for (int c = 0; c < C; c++) {
                    TT tmp = 0;
                    for (int o = -oset; o <= oset; o++) {
                        tmp += in_data[y * stride + (x + o) * C + c];
                    }
                    ot_data[y * stride + x * C + c] = (tmp + k_w - 1) / k_w;
                }
            }
        }
        for (int y = oset; y < input.height - oset; y++) {
            for (int x = oset; x < input.width - oset; x++) {
                for (int c = 0; c < C; c++) {
                    TT tmp = 0;
                    for (int o = -oset; o <= oset; o++) {
                        tmp += ot_data[(y + o) * stride + x * C + c];
                    }
                    ot_data[y * stride + x * C + c] = (tmp + k_w - 1) / k_w;
                }
            }
        }
        for (int y = input.height - oset; y < input.height; y++) {
            for (int x = 0; x < input.width; x++) {
                for (int c = 0; c < C; c++) {
                    ot_data[y * stride + x * C + c] = in_data[y * stride + x * C + c];
                }
            }
        }
        return output;
    }
}

//boxFilter
template <typename T, int C, int k_w>
Image<T, C> boxFilter(const Image<T, C>& input)
{
    return detail::_boxFilter<T, C, T, k_w>(input);
}
template <int k_w>
Image<uint8_t, 1> boxFilter(const Image<uint8_t, 1>& input)
{
    return detail::_boxFilter<uint8_t, 1, uint16_t, k_w>(input);
}
template <int k_w>
Image<uint8_t, 3> boxFilter(const Image<uint8_t, 3>& input)
{
    return detail::_boxFilter<uint8_t, 3, uint16_t, k_w>(input);
}

//grey2Rgb
template <typename T>
Image<T, 3> grey2Rgb(const Image<T, 1>& input)
{
    return detail::_grey2Rgb<T, T>(input);
}
template <>
Image<uint8_t, 3> grey2Rgb<uint8_t>(const Image<uint8_t, 1>& input)
{
    return detail::_grey2Rgb<uint8_t, uint16_t>(input);
}

//Rgb2grey
template <typename T>
Image<T, 1> Rgb2grey(const Image<T, 3>& input)
{
    return detail::_Rgb2grey<T, T>(input);
}
template <>
Image<uint8_t, 1> Rgb2grey(const Image<uint8_t, 3>& input)
{
    return detail::_Rgb2grey<uint8_t, uint16_t>(input);
}

//intImage
template <typename T, int C, typename TT>
Image<TT, C> intImage(const Image<T, C>& input)
{
    return detail::_intImageInc<T, C, TT>(input);
}
}