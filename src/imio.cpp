#include "image.h"
#include <string>
#include <algorithm>
#include <fstream>
#include <iomanip>

#pragma warning(disable : 4996)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace img {
	std::string getExtension(const std::string & str) {
		auto found = str.find_last_of(".");
		auto ext = (found == std::string::npos) ? "" : str.substr(found);
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
		return ext;
	}
	template <typename T, int C>
	Image<T,C> imread(const char * name) {
		auto ext = getExtension(name);

		Image<T, C> returnImage = {};
		int channels;
		if (ext == ".pfm") {
			std::ifstream file(name, std::ios::binary);
			std::string fileType;
			int width, height;
			float max;
			char tmp;

			file >> fileType >> width >> height >> max;
			auto res = img::Image<T, C>(width, height);
			returnImage = img::Image<T,C>(width, height);

			file.read(&tmp, 1); //eat the newline
			file.read((char*)res.data.get(), width*height * sizeof(T)*C);

			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					returnImage.ptr[y*width + x] = res.ptr[(height - 1 - y)*width + x];
				}
			}
		} else {
			returnImage.data = std::shared_ptr<T>((T*)stbi_load(name, &returnImage.width, &returnImage.height, &channels, C));
			returnImage.ptr = returnImage.data.get();
		}
		return returnImage;
	}

	template <typename T, int C>
	void imwrite(const char * name, Image<T, C> &img) {
		auto ext = getExtension(name);

		if (ext == ".png") {
			stbi_write_png(name, img.width, img.height, C, (unsigned char*)img.data.get(), 0);
		} else if (ext == ".tga") {						
			stbi_write_tga(name, img.width, img.height, C, (unsigned char*)img.data.get());
		} else if (ext == ".bmp") {						
			stbi_write_bmp(name, img.width, img.height, C, (unsigned char*)img.data.get());
		} else if (ext == ".hdr") {						
			stbi_write_hdr(name, img.width, img.height, C, (float*)img.data.get());
		} else if (ext == ".pfm") {
			auto res = img.copy();
			auto width = img.width;
			auto height = img.height;
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					res.ptr[y*width + x] = img.ptr[(height - 1 - y)*width + x];
				}
			}
			std::ofstream output(name, std::ofstream::binary | std::ofstream::out);
			output << "Pf\n";
			output << width << ' ' << height << '\n';
			output << std::fixed << std::setprecision(2) << -1.0 << '\n';

			output.write((char*)res.ptr, width*height * sizeof(float));
		}
	}
	template void imwrite(const char * name, Image<uint8_t,1> &img);
	template void imwrite(const char * name, Image<uint8_t, 3> &img);
	template void imwrite(const char * name, Image<uint8_t, 4> &img);
	template void imwrite(const char * name, Image<float, 4> &img);
	template void imwrite(const char * name, Image<float, 1> &img);// pfm only

	template Image<uint8_t, 1> imread(const char * name);
	template Image<uint8_t, 3> imread(const char * name);
	template Image<uint8_t, 4> imread(const char * name);
	template Image<float, 4> imread(const char * name);
	template Image<uint16_t, 1> imread(const char * name);
	template Image<uint16_t, 3> imread(const char * name);
	template Image<float, 1> imread(const char * name); // pfm only

}