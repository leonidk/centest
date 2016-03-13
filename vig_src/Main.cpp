#include "../src/imio.h"
#include <string>

int main(int argc, char* argv[])
{
    if(argc < 4) {
        printf("Usage: %s <fov> <input> <output>\n", argv[0]);
        return 1;
    }

    int hfov = atoi(argv[1]);
    auto input_fn = argv[2];
    auto output_fn = argv[3];

    auto img = img::imread<uint8_t,3>(input_fn);
    auto img_out = img::Image<uint8_t,3>(img.width,img.height);
    auto ptr = img.data.get();
    auto ptr2 = img_out.data.get();
    
    auto db = pow(cos(hfov*3.14159/360.0),4);
    printf("%lf\n",db);
    for (int y=0; y < img_out.height; y++) { 
        for (int x=0; x < img_out.width; x++) { 
            for(int c=0; c < 3; c++){
                auto xc = static_cast<float>(x);
                auto yc = static_cast<float>(y);
                float hw = img.width/2.0f;
                float hh = img.height/2.0f;
                auto xx = (xc - hw);
                auto yy = (yc - hh);
                //0.05
                auto dist = sqrt(xx*xx + yy*yy);
                auto maxDist = sqrt(hw*hw + hh*hh);
                auto weight = dist/maxDist;

                weight = tan(weight)/tan(1.0);

                ptr2[3*(y*img_out.width+x)+c] = weight*db*ptr[3*(y*img.width+x)+c] + (1-weight)*ptr[3*(y*img.width+x)+c];
            }
        }
    }
    img::imwrite(output_fn,img_out);
    return 0;
}
