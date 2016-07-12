#pragma once
#include "image.h"
#include <cstring>
namespace img {
    namespace detail {
        template <typename T, typename TT>
        Image<T, 3> _grey2Rgb(const Image<T, 1> & input){
            Image<T, 3> output(input.width, input.height);
            T* in_data = (T*)input.data.get();
            T* ot_data = (T*)output.data.get();
            auto stride = input.width * 3;
            for (int y = 0; y < input.height; y++) {
                for (int x = 0; x < input.width; x++) {
                    for (int c = 0; c < 3; c++){
                        ot_data[y*stride + x * 3 + c] = in_data[y*input.width + x];
                    }
                }
            }
            return output;
        }

        template <typename T, typename TT>
        Image<T, 1> _Rgb2grey(const Image<T, 3> & input){
            Image<T, 1> output(input.width, input.height);
            T* in_data = (T*)input.data.get();
            T* ot_data = (T*)output.data.get();
            auto stride = input.width * 3;
            for (int y = 0; y < input.height; y++) {
                for (int x = 0; x < input.width; x++) {
                    TT sum = 0;
                    for (int c = 0; c < 3; c++){
                        sum += in_data[y*stride + x * 3 + c];
                    }
                    ot_data[y*input.width + x] = (sum + 2) / 3;

                }
            }
            return output;
        }

        template <typename T, int C, typename TT>
        Image<TT, C> _intImageInc(const Image<T, C> & input){
            Image<TT, C> output(input.width, input.height);

            T* in_data = (T*)input.data.get();
            TT* ot_data = (TT*)output.data.get();
            for (int x = 0; x < input.width*C; x++) {
                ot_data[x] = in_data[x];
            }
            for (int y = 1; y < input.height; y++) {
                TT sum[C] = {};
                for (int x = 0; x < input.width; x++) {
                    for (int c = 0; c < C; c++){
                        sum[c] += in_data[(y - 1)*input.width*C + x*C + c];
                        ot_data[y*input.width*C + x*C + c] = sum[c] + ot_data[(y - 1)*input.width*C + x*C + c];
                    }
                }
            }
            return output;
        }
        template <typename T, int C, typename TT>
        Image<TT, C> _intImageEx(const Image<T, C> & input){
            Image<TT, C> output(input.width + 1, input.height + 1);

            T* in_data = (T*)input.data.get();
            TT* ot_data = (TT*)output.data.get();
            auto stride = C*(input.width + 1);
            for (int x = 0; x < stride; x++) {
                ot_data[x] = 0;
            }
            for (int y = 1; y < input.height + 1; y++) {
                ot_data[y*stride] = 0;
                TT sum[C] = {};
                for (int x = 0; x < input.width; x++) {
                    for (int c = 0; c < C; c++){
                        sum[c] += in_data[(y - 1)*input.width*C + x*C + c];
                        ot_data[y*stride + (x + 1)*C + c] = sum[c] + ot_data[(y - 1)*stride + (x + 1)*C + c];
                    }
                }
            }
            return output;
        }
        template <typename T, int C, typename TT, int k_w>
        Image<T, C> _boxFilter(const Image<T, C> & input){
            Image<T, C> output(input.width, input.height);
            Image<T, C> tmp(input.width, input.height);

            T* in_data = (T*)input.data.get();
            T* ot_data = (T*)output.data.get();
            T* tmp_data = (T*)tmp.data.get();

            memcpy(ot_data, in_data, sizeof(T)*input.width*input.height*C);
            auto stride = input.width*C;
            auto oset = k_w / 2;
            for (int y = oset; y < input.height - oset; y++) {
                for (int x = oset; x < input.width - oset; x++) {
                    for (int c = 0; c < C; c++) {
                        TT tmp = 0;
                        for (int o = -oset; o <= oset; o++) {
                            tmp += in_data[y * stride + (x + o)*C + c];
                        }
                        tmp_data[y * stride + x*C + c] = (tmp + k_w - 1) / k_w;
                    }
                }
            }
            for (int y = oset; y < input.height - oset; y++) {
                for (int x = oset; x < input.width - oset; x++) {
                    for (int c = 0; c < C; c++) {
                        TT tmp = 0;
                        for (int o = -oset; o <= oset; o++) {
                            tmp += tmp_data[(y + o) * stride + x*C + c];
                        }
                        ot_data[y * stride + x*C + c] = (tmp + k_w - 1) / k_w;
                    }
                }
            }
            return output;
        }
    }

    //domainTransform
    template<typename T, int C>
    Image<T, C> domainTransform(
        Image<T, C> input,
        Image<T, C> guide,
        const int iters,
        const float sigma_space,
        const float sigma_range) {
        auto ratio = sigma_space / sigma_range;

        Image<float, 1> ctx(input.width, input.height);
        Image<float, 1> cty(input.width, input.height);
        Image<float, 1> f_tmp(input.width, input.height);

        Image<float, C> out(input.width, input.height);
        Image<T, C> out_final(input.width, input.height);

        for (int i = 0; i < input.width*input.height*C; i++)
            out.ptr[i] = static_cast<float>(input.ptr[i]);

        //ctx
        for (int y = 0; y < input.height; y++) {
            for (int x = 0; x < input.width - 1; x++) {
                auto idx = C*(y*ctx.width + x);
                auto idxn = C*(y*ctx.width + x + 1);
                auto idxm = (y*ctx.width + x);

                float sum = 0;
                for (int c = 0; c < C; c++)
                    sum += std::abs(guide.ptr[idx + c] - guide.ptr[idxn + c]);
                ctx.ptr[idxm] = 1.0f + ratio*sum;
            }
        }

        //cty
        for (int x = 0; x < input.width; x++) {
            float sum = 0;
            for (int y = 0; y < input.height - 1; y++) {
                auto idx = C*(y*cty.width + x);
                auto idxn = C*((y + 1)*cty.width + x);
                auto idxm = (y*ctx.width + x);

                float sum = 0;
                for (int c = 0; c < C; c++)
                    sum += std::abs(guide.ptr[idx] - guide.ptr[idxn]);
                cty.ptr[idxm] = 1.0f + ratio*sum;
            }
        }

        // apply recursive filtering
        for (int i = 0; i < iters; i++) {
            auto sigma_H = sigma_space * sqrt(3.0f) * pow(2.0f, iters - i - 1) / sqrt(pow(4.0f, iters) - 1);
            auto alpha = exp(-sqrt(2.0f) / sigma_H);
            //horiz pass
            //generate f
            for (int y = 0; y < input.height; y++) {
                for (int x = 0; x < input.width - 1; x++) {
                    auto idx = y*input.width + x;
                    f_tmp.ptr[idx] = pow(alpha, ctx.ptr[idx]);
                }
            }
            //apply
            for (int y = 0; y < input.height; y++) {
                for (int x = 1; x < input.width; x++) {
                    auto idx = C*(y*input.width + x);
                    auto idxp = C*(y*input.width + x - 1);
                    auto idxpm = (y*input.width + x - 1);

                    float a = f_tmp.ptr[idxpm];
                    for (int c = 0; c < C; c++) {
                        float p = out.ptr[idx + c];
                        float pn = out.ptr[idxp + c];

                        out.ptr[idx + c] = p + a*(pn - p);
                    }
                }
                for (int x = input.width - 2; x >= 0; x--) {
                    auto idx = C*(y*input.width + x);
                    auto idxn = C*(y*input.width + x + 1);
                    auto idxm = (y*input.width + x);

                    float a = f_tmp.ptr[idxm];
                    for (int c = 0; c < C; c++) {
                        float p = out.ptr[idx + c];
                        float pn = out.ptr[idxn + c];

                        out.ptr[idx + c] = p + a*(pn - p);
                    }
                }
            }

            //vertical pass
            //generate f
            for (int y = 0; y < input.height - 1; y++) {
                for (int x = 0; x < input.width; x++) {
                    auto idx = y*input.width + x;
                    f_tmp.ptr[idx] = pow(alpha, cty.ptr[idx]);
                }
            }
            //apply
            for (int x = 0; x < input.width; x++) {
                for (int y = 1; y < input.height; y++) {
                    auto idx = C*(y*input.width + x);
                    auto idxp = C*((y - 1)*input.width + x);
                    auto idxpm = (y - 1)*input.width + x;

                    float a = f_tmp.ptr[idxpm];
                    for (int c = 0; c < C; c++) {
                        float p = out.ptr[idx + c];
                        float pn = out.ptr[idxp + c];

                        out.ptr[idx + c] = p + a*(pn - p);
                    }
                }
                for (int y = input.height - 2; y >= 0; y--) {
                    auto idx = C*(y*input.width + x);
                    auto idxn = C*((y + 1)*input.width + x);
                    auto idxm = y*input.width + x;

                    float a = f_tmp.ptr[idxm];
                    for (int c = 0; c < C; c++) {
                        float p = out.ptr[idx + c];
                        float pn = out.ptr[idxn + c];

                        out.ptr[idx + c] = p + a*(pn - p);
                    }
                }
            }
        }
        for (int i = 0; i < input.width*input.height*C; i++)
            out_final.ptr[i] = (T)(out.ptr[i] + 0.5f);

        return out_final;
    }
    template<typename T, int C>
    Image<T, 1> domainTransformDepth(
        Image<T, C> input,
        Image<T, 1> guide,
        const int iters,
        const float sigma_space,
        const float sigma_range) {
        auto ratio = sigma_space / sigma_range;

        Image<float, 1> ctx(input.width, input.height);
        Image<float, 1> cty(input.width, input.height);
        Image<float, 1> f_tmp(input.width, input.height);

        Image<float, C> out(input.width, input.height);
        Image<T, C> out_final(input.width, input.height);

        for (int i = 0; i < input.width*input.height*C; i++)
            out.ptr[i] = static_cast<float>(input.ptr[i]);

        //ctx
        for (int y = 0; y < input.height; y++) {
            int value = 0;
            int steps = 0;
            bool filling = false;
            for (int x = 0; x < input.width - 1; x++) {
                auto idx = y*ctx.width + x;
                auto idxn = y*ctx.width + x + 1;

                auto p = guide.ptr[idx];
                auto pn = guide.ptr[idxn];
                if (filling && p == USHRT_MAX) {
                    steps++;
                    continue;
                }
                if (filling) {
                    auto diff = std::abs(p - value + steps) / steps;
                    for (int i = 0; i < steps + 1; i++) {
                        ctx.ptr[y*ctx.width + x - i] = 1.0f + ratio*diff;
                    }
                    filling = false;
                    continue;
                }
                if (!filling && pn == USHRT_MAX) {
                    value = p;
                    steps = 1;
                    filling = true;
                    continue;
                }
                ctx.ptr[idx] = 1.0f + ratio*std::abs(p - pn);
            }
            if (filling) {
                for (int i = 0; i < steps + 1; i++) {
                    ctx.ptr[y*ctx.width + input.width - 1 - i] = 1.0f + USHRT_MAX / steps;
                }
            }
        }

        //cty
        for (int x = 0; x < input.width; x++) {
            int value = 0;
            int steps = 0;
            bool filling = false;
            for (int y = 0; y < input.height - 1; y++) {
                auto idx = y*cty.width + x;
                auto idxn = (y + 1)*cty.width + x;

                auto p = guide.ptr[idx];
                auto pn = guide.ptr[idxn];

                if (filling && p == USHRT_MAX) {
                    steps++;
                    continue;
                }
                if (filling) {
                    auto diff = std::abs(p - value + steps) / steps;
                    for (int i = 0; i < steps + 1; i++) {
                        cty.ptr[(y - i)*cty.width + x] = 1.0f + ratio*diff;
                    }
                    filling = false;
                    continue;
                }
                if (!filling && pn == USHRT_MAX) {
                    value = p;
                    steps = 1;
                    filling = true;
                    continue;
                }
                cty.ptr[idx] = 1.0f + ratio*std::abs(p - pn);
            }
            if (filling) {
                for (int i = 0; i < steps + 1; i++) {
                    cty.ptr[(input.height - 1 - i)*cty.width + x] = 1.0f + ratio*USHRT_MAX / steps;
                }
            }
        }

        // apply recursive filtering
        for (int i = 0; i < iters; i++) {
            auto sigma_H = sigma_space * sqrt(3.0f) * pow(2.0f, iters - i - 1) / sqrt(pow(4.0f, iters) - 1);
            auto alpha = exp(-sqrt(2.0f) / sigma_H);
            //horiz pass
            //generate f
            for (int y = 0; y < input.height; y++) {
                for (int x = 0; x < input.width - 1; x++) {
                    auto idx = y*input.width + x;
                    f_tmp.ptr[idx] = pow(alpha, ctx.ptr[idx]);
                }
            }
            //apply
            for (int y = 0; y < input.height; y++) {
                for (int x = 1; x < input.width; x++) {
                    auto idx = C*(y*input.width + x);
                    auto idxp = C*(y*input.width + x - 1);
                    auto idxpm = (y*input.width + x - 1);

                    float a = f_tmp.ptr[idxpm];
                    for (int c = 0; c < C; c++) {
                        float p = out.ptr[idx + c];
                        float pn = out.ptr[idxp + c];
                        float diff = (pn - p);

                        if (p == USHRT_MAX && pn == USHRT_MAX) {
                        }
                        else if (p == USHRT_MAX) {
                            out.ptr[idx + c] = pn;
                        }
                        else if (pn == USHRT_MAX) {
                            out.ptr[idx + c] = p;
                        }
                        else {
                            out.ptr[idx + c] = p + a*diff;
                        }
                    }
                }
                for (int x = input.width - 2; x >= 0; x--) {
                    auto idx = C*(y*input.width + x);
                    auto idxn = C*(y*input.width + x + 1);
                    auto idxm = (y*input.width + x);

                    float a = f_tmp.ptr[idxm];
                    for (int c = 0; c < C; c++) {
                        float p = out.ptr[idx + c];
                        float pn = out.ptr[idxn + c];
                        float diff = (pn - p);

                        if (p == USHRT_MAX && pn == USHRT_MAX) {
                        }
                        else if (p == USHRT_MAX) {
                            out.ptr[idx + c] = pn;
                        }
                        else if (pn == USHRT_MAX) {
                            out.ptr[idx + c] = p;
                        }
                        else {
                            out.ptr[idx + c] = p + a*diff;
                        }
                    }
                }
            }

            //vertical pass
            //generate f
            for (int y = 0; y < input.height - 1; y++) {
                for (int x = 0; x < input.width; x++) {
                    auto idx = y*input.width + x;
                    f_tmp.ptr[idx] = pow(alpha, cty.ptr[idx]);
                }
            }
            //apply
            for (int x = 0; x < input.width; x++) {
                for (int y = 1; y < input.height; y++) {
                    auto idx = C*(y*input.width + x);
                    auto idxp = C*((y - 1)*input.width + x);
                    auto idxpm = (y - 1)*input.width + x;

                    float a = f_tmp.ptr[idxpm];
                    for (int c = 0; c < C; c++) {
                        float p = out.ptr[idx + c];
                        float pn = out.ptr[idxp + c];
                        float diff = (pn - p);

                        if (p == USHRT_MAX && pn == USHRT_MAX) {
                        }
                        else if (p == USHRT_MAX) {
                            out.ptr[idx + c] = pn;
                        }
                        else if (pn == USHRT_MAX) {
                            out.ptr[idx + c] = p;
                        }
                        else {
                            out.ptr[idx + c] = p + a*diff;
                        }
                    }
                }
                for (int y = input.height - 2; y >= 0; y--) {
                    auto idx = C*(y*input.width + x);
                    auto idxn = C*((y + 1)*input.width + x);
                    auto idxm = y*input.width + x;

                    float a = f_tmp.ptr[idxm];
                    for (int c = 0; c < C; c++) {
                        float p = out.ptr[idx + c];
                        float pn = out.ptr[idxn + c];
                        float diff = (pn - p);

                        if (p == USHRT_MAX && pn == USHRT_MAX) {
                        }
                        else if (p == USHRT_MAX) {
                            out.ptr[idx + c] = pn;
                        }
                        else if (pn == USHRT_MAX) {
                            out.ptr[idx + c] = p;
                        }
                        else {
                            out.ptr[idx + c] = p + a*diff;
                        }
                    }
                }
            }
        }
        for (int i = 0; i < input.width*input.height*C; i++)
            out_final.ptr[i] = (T)(out.ptr[i] + 0.5f);

        return out_final;
    }
    Image<uint16_t, 1> domainTransformJoint(
        Image<uint16_t, 1> input,
        Image<uint8_t, 3> guide,
        const int iters,
        const float sigma_space,
        const float sigma_range) {
        auto ratio = sigma_space / sigma_range;

        Image<float, 1> ctx(input.width, input.height);
        Image<float, 1> cty(input.width, input.height);
        Image<float, 1> f_tmp(input.width, input.height);

        Image<float, 1> out(input.width, input.height);
        Image<uint16_t, 1> out_final(input.width, input.height);

        for (int i = 0; i < input.width*input.height; i++)
            out.ptr[i] = static_cast<float>(input.ptr[i]);

        //ctx
        for (int y = 0; y < input.height; y++) {
            for (int x = 0; x < input.width - 1; x++) {
                auto idx = 3 * (y*ctx.width + x);
                auto idxn = 3 * (y*ctx.width + x + 1);
                auto idxm = (y*ctx.width + x);

                float sum = 0;
                for (int c = 0; c < 3; c++)
                    sum += std::abs(guide.ptr[idx + c] - guide.ptr[idxn + c]);
                ctx.ptr[idxm] = 1.0f + ratio*sum;
            }
        }

        //cty
        for (int x = 0; x < input.width; x++) {
            float sum = 0;
            for (int y = 0; y < input.height - 1; y++) {
                auto idx = 3 * (y*cty.width + x);
                auto idxn = 3 * ((y + 1)*cty.width + x);
                auto idxm = (y*ctx.width + x);

                float sum = 0;
                for (int c = 0; c < 3; c++)
                    sum += std::abs(guide.ptr[idx + c] - guide.ptr[idxn + c]);
                cty.ptr[idxm] = 1.0f + ratio*sum;
            }
        }

        // apply recursive filtering
        for (int i = 0; i < iters; i++) {
            auto sigma_H = sigma_space * sqrt(3.0f) * pow(2.0f, iters - i - 1) / sqrt(pow(4.0f, iters) - 1);
            auto alpha = exp(-sqrt(2.0f) / sigma_H);
            //horiz pass
            //generate f
            for (int y = 0; y < input.height; y++) {
                for (int x = 0; x < input.width - 1; x++) {
                    auto idx = y*input.width + x;
                    f_tmp.ptr[idx] = pow(alpha, ctx.ptr[idx]);
                }
            }
            //apply
            for (int y = 0; y < input.height; y++) {
                for (int x = 1; x < input.width; x++) {
                    auto idx = (y*input.width + x);
                    auto idxp = (y*input.width + x - 1);

                    float a = f_tmp.ptr[idxp];
                    float p = out.ptr[idx];
                    float pn = out.ptr[idxp];
                    float diff = (pn - p);

                    if (p == USHRT_MAX && pn == USHRT_MAX) {
                    }
                    else if (p == USHRT_MAX) {
                        out.ptr[idx] = pn;
                    }
                    else if (pn == USHRT_MAX) {
                        out.ptr[idx] = p;
                    }
                    else {
                        out.ptr[idx] = p + a*diff;
                    }

                }
                for (int x = input.width - 2; x >= 0; x--) {
                    auto idx = (y*input.width + x);
                    auto idxn = (y*input.width + x + 1);

                    float a = f_tmp.ptr[idx];
                    float p = out.ptr[idx];
                    float pn = out.ptr[idxn];
                    float diff = (pn - p);

                    if (p == USHRT_MAX && pn == USHRT_MAX) {
                    }
                    else if (p == USHRT_MAX) {
                        out.ptr[idx] = pn;
                    }
                    else if (pn == USHRT_MAX) {
                        out.ptr[idx] = p;
                    }
                    else {
                        out.ptr[idx] = p + a*diff;
                    }

                }
            }

            //vertical pass
            //generate f
            for (int y = 0; y < input.height - 1; y++) {
                for (int x = 0; x < input.width; x++) {
                    auto idx = y*input.width + x;
                    f_tmp.ptr[idx] = pow(alpha, cty.ptr[idx]);
                }
            }
            //apply
            for (int x = 0; x < input.width; x++) {
                for (int y = 1; y < input.height; y++) {
                    auto idx = (y*input.width + x);
                    auto idxp = ((y - 1)*input.width + x);

                    float a = f_tmp.ptr[idxp];
                    float p = out.ptr[idx];
                    float pn = out.ptr[idxp];
                    float diff = (pn - p);

                    if (p == USHRT_MAX && pn == USHRT_MAX) {
                    }
                    else if (p == USHRT_MAX) {
                        out.ptr[idx] = pn;
                    }
                    else if (pn == USHRT_MAX) {
                        out.ptr[idx] = p;
                    }
                    else {
                        out.ptr[idx] = p + a*diff;
                    }

                }
                for (int y = input.height - 2; y >= 0; y--) {
                    auto idx = (y*input.width + x);
                    auto idxn = ((y + 1)*input.width + x);
                    auto idxm = y*input.width + x;

                    float a = f_tmp.ptr[idxm];
                    float p = out.ptr[idx];
                    float pn = out.ptr[idxn];
                    float diff = (pn - p);

                    if (p == USHRT_MAX && pn == USHRT_MAX) {
                    }
                    else if (p == USHRT_MAX) {
                        out.ptr[idx] = pn;
                    }
                    else if (pn == USHRT_MAX) {
                        out.ptr[idx] = p;
                    }
                    else {
                        out.ptr[idx] = p + a*diff;
                    }

                }
            }
        }
        for (int i = 0; i < input.width*input.height; i++)
            out_final.ptr[i] = (uint16_t)(out.ptr[i] + 0.5f);

        return out_final;
    }
    //boxFilter
    template <typename T, int C, int k_w>
    Image<T, C> boxFilter(const Image<T, C> & input){
        return detail::_boxFilter<T, C, T, k_w>(input);
    }
    template <int k_w>
    Image<uint8_t, 1> boxFilter(const Image<uint8_t, 1> & input){
        return detail::_boxFilter<uint8_t, 1, uint16_t, k_w>(input);
    }
    template <int k_w>
    Image<uint8_t, 3> boxFilter(const Image<uint8_t, 3> & input){
        return detail::_boxFilter<uint8_t, 3, uint16_t, k_w>(input);
    }

    //grey2Rgb
    template <typename T>
    Image<T, 3> grey2Rgb(const Image<T, 1> & input){
        return detail::_grey2Rgb<T, T>(input);
    }
    template <>
    Image<uint8_t, 3> grey2Rgb<uint8_t>(const Image<uint8_t, 1> & input){
        return detail::_grey2Rgb<uint8_t, uint16_t>(input);
    }

    //Rgb2grey
    template <typename T>
    Image<T, 1> Rgb2grey(const Image<T, 3> & input){
        return detail::_Rgb2grey<T, T>(input);
    }
    template <>
    Image<uint8_t, 1> Rgb2grey(const Image<uint8_t, 3> & input){
        return detail::_Rgb2grey<uint8_t, uint16_t>(input);
    }

    //intImage
    template <typename T, int C, typename TT>
    Image<TT, C> intImage(const Image<T, C> & input){
        return detail::_intImageInc<T, C, TT>(input);
    }

}
