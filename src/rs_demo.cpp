#include "bmMatch.h"
#include "cMatch.h"
#include "imio.h"
#include "r200Match.h"
#include "sgbmMatch.h"

#include "image_filter.h"
#include "cam_util.h"

#include <string>
#include <fstream>
#include <chrono>
#include <cmath>
#include <iostream>
#include <librealsense\rs.hpp>


const int MAXDISP = 64;
const int MULDISP = 32;
int main(int argc, char* argv[])
{
    rs::context ctx;
    if (ctx.get_device_count() == 0) throw std::runtime_error("No device detected. Is it plugged in?");
    rs::device & dev = *ctx.get_device(0);
    try {
        dev.enable_stream(rs::stream::depth, 480, 360, rs::format::disparity16, 30);
        dev.enable_stream(rs::stream::color, 640, 480, rs::format::rgb8, 30);
        dev.enable_stream(rs::stream::infrared, 492, 372, rs::format::y8, 30);
        dev.enable_stream(rs::stream::infrared2, 492, 372, rs::format::y8, 30);

        // Start our device
        dev.start();

        dev.set_option(rs::option::r200_emitter_enabled, true);
        dev.set_option(rs::option::r200_lr_auto_exposure_enabled, true);
        //rs_apply_depth_control_preset((rs_device*)&dev, 2); // 2 = low, 4=opt, 5 = high
    }
    catch (std::exception & e) {
        std::cout << e.what() << std::endl;
        return 1;
    }

    auto intrin = dev.get_stream_intrinsics(rs::stream::depth);
    auto extrin = dev.get_extrinsics(rs::stream::infrared2, rs::stream::infrared);

    auto fB = intrin.fx * extrin.translation[0] * 1000.0f;
    int counter = 0;
    auto w = dev.get_stream_width(rs::stream::depth);
    auto h = dev.get_stream_height(rs::stream::depth);
    auto iw = dev.get_stream_width(rs::stream::infrared);
    auto ih = dev.get_stream_height(rs::stream::infrared);
    stereo::R200Match cm(iw, ih, MAXDISP, MULDISP);
    auto himg4 = img::Image<uint8_t, 3>(w, h);
    auto himg5 = img::Image<uint8_t, 3>(iw, ih);
    img::Img<uint16_t> disp_s(iw, ih);
    memset(disp_s.ptr, 0, iw*ih*sizeof(uint16_t));
    do {
        dev.wait_for_frames();
        //depth based
        {
            auto disp = dev.get_frame_data(rs::stream::depth);
            auto left = dev.get_frame_data(rs::stream::infrared);
            auto right = dev.get_frame_data(rs::stream::infrared2);


            auto dimg = img::Img<uint16_t>(w, h, (uint16_t*)disp);
            auto limg = img::Img<uint8_t>(iw, ih, (uint8_t*)left);
            auto rimg = img::Img<uint8_t>(iw, ih, (uint8_t*)right);

            cm.match(limg, rimg, disp_s);


            util::ConvertDepthToRGBUsingHistogram(himg4.ptr, dimg.ptr, w, h, 0.1f, 0.625f);
            util::ConvertDepthToRGBUsingHistogram(himg5.ptr, disp_s.ptr, iw, ih, 0.1f, 0.625f);
            img::imshow("ds4", himg4);
            img::imshow("sw", himg5);
        }

    } while (img::getKey() != 'q');

    return 0;
}
