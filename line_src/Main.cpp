#include "../src/imio.h"
#include <string>
#include <cstring>

void coord(double x,double y, int width, int height, double & xo, double & yo)
{
    auto hw = width/2.0;
    auto hh = height/2.0;
    auto xx = (x - hw)/hw;
    auto yy = (y - hh)/hh;
    //auto dist = sqrt(xx*xx + yy*yy);
    //auto maxDist = sqrt(hw*hw + hh*hh);
    //auto weight = dist/maxDist;
    //weight = tan(weight)/tan(1.0);
    //ptr2[3*(y*img_out.width+x)+c] = ptr[3*(y*img.width+x)+c];
    
    //xo = cbrt(xx)*hw + hw;
    xo = xx*hw+hw;
    yo = cbrt(yy)*hh + hh;
    //yo = yy*hh+hh;
}

void draw_at(uint8_t *ptr, int x, int y, int width, int height)
{
    double eta = 1e-6;
    double xp,yp,xn,yn;
    coord(x-0.5-eta,y-0.5+eta,width,height,xp,yp);
    coord(x+0.5-eta,y+0.5+eta,width,height,xn,yn);
    xn = std::round(xn);
    yn = std::round(yn);
    xp = std::round(xp);
    yp = std::round(yp);
    //printf("%lf %lf %lf %lf\n",xp,yp,xn,yn);
    for(int xi = xp; xi < xn; xi++){
        for(int yi = yp; yi < yn; yi++){
            for(int c=0; c < 3; c++){
                auto xo = std::min(width-1,std::max(0,xi));
                auto yo = std::min(height-1,std::max(0,yi));
        
                ptr[3*(yo*width+xo)+c] = 0;
            }
        }
    }
}

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
    memset(ptr2,0xFF,img.width*img.height*3);    
    for (int y=0; y < img_out.height; y+=40) { 
        for (int x=0; x < img_out.width; x++) { 
            draw_at(ptr2,x,y,img.width,img.height);
        }
    }
    for (int y=0; y < img_out.height; y++) { 
        for (int x=0; x < img_out.width; x+=40) { 
            draw_at(ptr2,x,y,img.width,img.height);
        }
    }
    img::imwrite(output_fn,img_out);
    return 0;
}
