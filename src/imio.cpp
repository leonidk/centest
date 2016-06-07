#include "image.h"
#include <algorithm>
#include <string>
#pragma warning(disable : 4996)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace img {
std::string getExtension(const std::string& str)
{
    auto found = str.find_last_of(".");
    auto ext = (found == std::string::npos) ? "" : str.substr(found);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}
template <typename T, int C>
Image<T, C> imread(const char* name)
{
    Image<T, C> returnImage = {};
    int channels;
    returnImage.data = std::shared_ptr<T>((T*)stbi_load(name, &returnImage.width, &returnImage.height, &channels, C));
    returnImage.ptr = returnImage.data.get();
    return returnImage;
}

template <typename T, int C>
void imwrite(const char* name, const Image<T, C>& img)
{
    auto ext = getExtension(name);

    if (ext == ".png") {
        stbi_write_png(name, img.width, img.height, C, (unsigned char*)img.data.get(), 0);
    } else if (ext == ".tga") {
        stbi_write_tga(name, img.width, img.height, C, (unsigned char*)img.data.get());
    } else if (ext == ".bmp") {
        stbi_write_bmp(name, img.width, img.height, C, (unsigned char*)img.data.get());
    } else if (ext == ".hdr") {
        stbi_write_hdr(name, img.width, img.height, C, (float*)img.data.get());
    }
}
template void imwrite(const char* name, const Image<uint8_t, 1>& img);
template void imwrite(const char* name, const Image<uint8_t, 3>& img);
template void imwrite(const char* name, const Image<uint8_t, 4>& img);
template void imwrite(const char* name, const Image<float, 4>& img);

template Image<uint8_t, 1> imread(const char* name);
template Image<uint8_t, 3> imread(const char* name);
template Image<uint8_t, 4> imread(const char* name);
template Image<float, 4> imread(const char* name);
}