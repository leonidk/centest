#include "image.h"
#include <GLFW/glfw3.h>
#include <mutex>
#include <unordered_map>
#include <string>
#include <stdint.h>
#include <memory>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>
#pragma comment( lib, "opengl32" )
namespace img {
	template <int C>
	inline int getGLChan();

	template <>
	inline int getGLChan<1>() { return GL_LUMINANCE; }
	template <>
	inline int getGLChan<2>() { return GL_LUMINANCE_ALPHA; }
	template <>
	inline int getGLChan<3>() { return GL_RGB; }
	template <>
	inline int getGLChan<4>() { return GL_RGBA; }

	template <typename T>
	inline int getGLType();

	template <>
	inline int getGLType<uint8_t>() { return GL_UNSIGNED_BYTE; }
	template <>
	inline int getGLType<int8_t>() { return GL_BYTE; }
	template <>
	inline int getGLType<uint16_t>() { return GL_UNSIGNED_SHORT; }
	template <>
	inline int getGLType<int16_t>() { return GL_SHORT; }
	template <>
	inline int getGLType<uint32_t>() { return GL_UNSIGNED_INT; }
	template <>
	inline int getGLType<int32_t>() { return GL_INT; }
	template <>
	inline int getGLType<float>() { return GL_FLOAT; }
	template <>
	inline int getGLType<double>() { return GL_DOUBLE; }
	struct glfwState {
		glfwState() { glfwInit(); }
		~glfwState() { glfwTerminate(); }
	};

	glfwState g_glfw_state;

	char g_key;
	bool g_key_update = false;

	std::mutex g_mutex;
	struct winState {
		GLFWwindow* win;
		GLuint tex;
		int width, height;
	};
	std::unordered_map<std::string, winState> g_windows;

	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		std::unique_lock<std::mutex> lock(g_mutex);

		if (action == GLFW_PRESS) {
			g_key = tolower(key);
			g_key_update = true;
		}
	}
	static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
	{
		glfwMakeContextCurrent(window);
		for (const auto & win : g_windows) {
			if (win.second.win == window) {
				auto wScale = ((float)width) / ((float)win.second.width);
				auto hScale = ((float)height) / ((float)win.second.height);
				auto minScale = (wScale < hScale) ? wScale : hScale;
				int wShift = (int)nearbyint((width - minScale*win.second.width) / 2.0f);
				int hShift = (int)nearbyint((height - minScale*win.second.height) / 2.0f);

				glViewport(wShift, hShift, (int)nearbyint(win.second.width*minScale), (int)nearbyint(win.second.height*minScale));
			}
		}
	}
	template <typename T, int C>
	void imshow(const char * name, const Image<T,C> & img){
		std::unique_lock<std::mutex> lock(g_mutex);
		std::string s_name(name);
		if (img.data.get() == NULL || img.height*img.width == 0)
			return;
		if (g_windows.find(s_name) == g_windows.end())
		{
			GLuint tex;
			glGenTextures(1, &tex);
			g_windows[s_name] = { glfwCreateWindow(img.width, img.height, s_name.c_str(), NULL, NULL), tex, img.width, img.height };
			glfwSetKeyCallback(g_windows[s_name].win, key_callback);
			glfwSetFramebufferSizeCallback(g_windows[s_name].win, framebuffer_size_callback);
		}
		auto window = g_windows[s_name];
		glfwMakeContextCurrent(window.win);
		glPushAttrib(GL_ALL_ATTRIB_BITS);
		glPushMatrix();
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_ALPHA);
		glClearColor(0, 0, 0, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glBindTexture(GL_TEXTURE_2D, window.tex);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.width, img.height, 0, getGLChan<C>(), getGLType<T>(), img.data.get());

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		glBegin(GL_QUADS);
		glTexCoord2f(0, 1);	 glVertex2f(-1, -1);
		glTexCoord2f(1, 1);	 glVertex2f(1, -1);
		glTexCoord2f(1, 0);	 glVertex2f(1, 1);
		glTexCoord2f(0, 0);	 glVertex2f(-1, 1);
		glEnd();
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_ALPHA);
		glPopMatrix();
		glPopAttrib();
	}


	char getKey(bool wait)
	{
		std::unique_lock<std::mutex> lock(g_mutex);

		std::vector<std::string> toDelete;
		for (const auto & win : g_windows) {
			if (glfwWindowShouldClose(win.second.win)) {
				toDelete.push_back(win.first);
			}
			else {
				glfwMakeContextCurrent(win.second.win);
				glfwSwapBuffers(win.second.win);
				lock.unlock();
				if (wait)
					do {
						glfwWaitEvents();
					} while (!g_key_update);
				else {
					glfwPollEvents();
				}
				lock.lock();

				if (!g_key_update) {
					g_key = '\0';
				}
			}
		}
		g_key_update = false;
		for (const auto & name : toDelete) {
			glfwDestroyWindow(g_windows[name].win);
			glDeleteTextures(1, &g_windows[name].tex);
			g_windows.erase(name);
		}
		return g_key;
	}
	template void imshow(const char * name, const Image<uint8_t,1> &img);
	template void imshow(const char * name, const Image<uint8_t, 3> &img);
	template void imshow(const char * name, const Image<uint8_t, 4> &img);
    template void imshow(const char * name, const Image<uint16_t, 1> &img);
    template void imshow(const char * name, const Image<float, 3> &img);

}
