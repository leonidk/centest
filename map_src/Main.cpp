#include "../src/imio.h"
#include <string>

int main(int argc, char* argv[])
{
    if(argc < 4) {
        printf("Usage: %s <scale> <cubic> <input> <output>\n", argv[0]);
        return 1;
    }

    int scale = atoi(argv[1]);
    int cubic = atoi(argv[2]);
    auto input_fn = argv[3];
    auto output_fn = argv[4];

	const int channels = 3;

    auto img = img::imread<uint8_t,channels>(input_fn);
    auto img_out = img::Image<uint8_t,channels>(img.width/scale,img.height/scale);
    auto ptr2 = img_out.data.get();
    for (int y=0; y < img_out.height; y++) { 
        for (int x=0; x < img_out.width; x++) { 
            for(int c=0; c < channels; c++){
                auto xc = static_cast<float>(x*scale);
                auto yc = static_cast<float>(y*scale);
                float hw = img.width/2.0f;
                float hh = img.height/2.0f;
                auto xx = (xc - hw)/hw;
                auto yy = (yc - hh)/hh;
                if(cubic&1) {  
                    //xx = xx * xx * xx;
					xx = sin(xx) * (1.0 / sin(1.0));
                } 
                if(cubic&2) {  
                    //yy = yy * yy * yy;
					yy = sin(yy) * (1.0 / sin(1.0));

                } 
                xc = xx*hw + hw;
                yc = yy*hh + hh;
                ptr2[channels*(y*img_out.width+x)+c] = img.sample(xc,yc,c);
            }
        }
    }
    img::imwrite(output_fn,img_out);
    return 0;
}
