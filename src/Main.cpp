#include "bmMatch.h"
#include "cMatch.h"
#include "imio.h"
#include "r200Match.h"
#include "sgbmMatch.h"

#include "image_filter.h"

#include <string>
#include <fstream>
#include <chrono>

const int resizeGT = 0;

img::Img<float> loadDisparityMap(std::string filename)
{
    std::ifstream file(filename, std::ios::binary);
    std::string fileType;
    int width, height;
    float max;
    char tmp;

    file >> fileType >> width >> height >> max;
    auto res = img::Img<float>(width, height);

    file.read(&tmp, 1); //eat the newline
    file.read((char*)res.data.get(), width*height*sizeof(float));
    
    auto r2 = res.copy();
    
    auto r2ptr = r2.data.get();
    auto rptr = res.data.get();
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            rptr[y*width + x] = r2ptr[(height - 1 - y)*width + x];
        }
    }
    if (resizeGT) {
        width /= 4;
        height /= 4;
        res = img::Image<float, 1>(width, height);
        rptr = res.data.get();
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                rptr[y*width + x] = r2ptr[(height*4 - 1 - 4*y)*width*4 + x*4]/4.0f;
            }
        }
    }
    return res;
}

int main(int argc, char* argv[])
{
    if (argc < 3)
        return 1;

    img::Img<float> gt;
    if (argc >= 4)
        gt = loadDisparityMap(argv[3]);

    std::string leftFile(argv[1]);
    std::string rightFile(argv[2]);

    auto left = img::imread<uint8_t, 3>(argv[1]);
    auto right = img::imread<uint8_t, 3>(argv[2]);

    auto left_g = img::Rgb2grey(left);
    auto right_g = img::Rgb2grey(right);
    stereo::sgbmMatch cm(left.width, left.height, 270, 3);

    auto startTime = std::chrono::steady_clock::now();
    auto disp = cm.match(left_g, right_g);
    auto endTime = std::chrono::steady_clock::now();;

    auto ot = img::Image<uint8_t, 1>(disp.width, disp.height);
    auto dptr = disp.data.get();
    auto optr = ot.data.get();
    memset(optr, 0, disp.width * disp.height);

    for (int i = 0; i < ot.width * ot.height; i++) {
        optr[i] = (uint8_t)std::round(dptr[i] * 254.0 / (270.0 * 3));
    }
    double mse = 0;
    double count = 0;
    auto sqr = [](double a) { return a * a; };
    if (gt.data.get() != nullptr) {
        auto gptr = gt.data.get();
        for (int i = 0; i < ot.width * ot.height; i++) {
            double pred =  dptr[i]/3.0;
            double corr = gptr[i];
            if (isfinite(corr)) {
                mse += sqr(pred -corr);
                count++;
            }
        }
    }
    img::imwrite("disp-out.png", ot);
    printf("\n\n %d seconds\n", std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count());
    printf("\n\n %lf average error\n", sqrt(mse / count));

    return 0;
}
