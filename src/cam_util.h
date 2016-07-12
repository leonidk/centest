#pragma once
#include <array>
#include <cstdint>

namespace util {
    template<class T> T clamp(T a, T mn, T mx) { return std::max(std::min(a, mx), mn); }

    template <typename T, bool clamp, int inputMin, int inputMax>
    inline T remapInt(T value, float outputMin, float outputMax)
    {
        T invVal = 1.0f / (inputMax - inputMin);
        T outVal = (invVal*(value - inputMin) * (outputMax - outputMin) + outputMin);
        if (clamp)
        {
            if (outputMax < outputMin)
            {
                if (outVal < outputMax) outVal = outputMax;
                else if (outVal > outputMin) outVal = outputMin;
            }
            else
            {
                if (outVal > outputMax) outVal = outputMax;
                else if (outVal < outputMin) outVal = outputMin;
            }
        }
        return outVal;
    }

    std::array<int, 3> hsvToRgb(double h, double s, double v) {

        std::array<int, 3> rgb;

        double r, g, b;

        int i = int(h * 6);
        double f = h * 6 - i;
        double p = v * (1 - s);
        double q = v * (1 - f * s);
        double t = v * (1 - (1 - f) * s);

        switch (i % 6){
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
        }

        rgb[0] = uint8_t(clamp((float)r * 255.0f, 0.0f, 255.0f));
        rgb[1] = uint8_t(clamp((float)g * 255.0f, 0.0f, 255.0f));
        rgb[2] = uint8_t(clamp((float)b * 255.0f, 0.0f, 255.0f));

        return rgb;

    }

    // from Graphene
    void ConvertDepthToRGBUsingHistogram(uint8_t img[], const uint16_t depthImage[], int width, int height, const float nearHue, const float farHue)
    {
        // Produce a cumulative histogram of depth values
        int histogram[256 * 256] = { 1 };

        for (int i = 0; i < width * height; ++i)
        {
            auto d = depthImage[i];
            if (d && d != USHRT_MAX) ++histogram[d];
        }

        for (int i = 1; i < 256 * 256; i++)
        {
            histogram[i] += histogram[i - 1];
        }

        // Remap the cumulative histogram to the range [0-256]
        for (int i = 1; i < 256 * 256; i++)
        {
            histogram[i] = (histogram[i] << 8) / histogram[256 * 256 - 1];
        }

        auto rgb = img;
        for (int i = 0; i < width * height; i++)
        {
            // For valid depth values (depth > 0) 
            uint16_t d = depthImage[i];
            if (d && d != USHRT_MAX)
            {
                auto t = histogram[d]; // Use the histogram entry (in the range of [0-256]) to interpolate between nearColor and farColor
                std::array<int, 3> returnRGB = { 0, 0, 0 };
                returnRGB = hsvToRgb(remapInt<float, true, 0, 255>(t, nearHue, farHue), 1.f, 1.f);
                *rgb++ = returnRGB[0];
                *rgb++ = returnRGB[1];
                *rgb++ = returnRGB[2];
            }
            // Use black pixels for invalid values (depth == 0)
            else
            {
                *rgb++ = 0;
                *rgb++ = 0;
                *rgb++ = 0;
            }
        }
    }
}