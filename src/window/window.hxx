#ifndef _INCLUDE_WINDOW_H
#define _INCLUDE_WINDOW_H
#include <memory>
#include <array>
#include <list>
#include <functional>
#include <iostream>
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
// About OpenGL function loaders: modern OpenGL doesn't have a standard header file and requires individual function pointers to be loaded manually.
// Helper libraries are often used for this purpose! Here we are supporting a few common ones: gl3w, glew, glad.
// You may use another loader/header of your choice (glext, glLoadGen, etc.), or chose to manually implement your own.
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>    // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>    // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h>  // Initialize with gladLoadGL()
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>
namespace ui {
	using namespace std;
	struct window_t {
		using render_fun = std::function<bool(ui::window_t*, mod::hvg::control::hvg_serial_t*, ui::log::Display_Log*)>;
		GLFWwindow* wnd;
		int x, y, w, h;
		ImVec4 clear_color;
		char title[50];
		list<render_fun> renders;
	};
	static void glfw_error_callback(int error, const char* description);
	bool init(window_t* win, int x, int y, int w, int h, array<float, 4> background_color, const char* title, std::function<bool(ui::window_t*, mod::hvg::control::hvg_serial_t*, ui::log::Display_Log*)> function_ptr);

	void clean_up(window_t* win);
	void new_frame(window_t* win);
	void draw(window_t* win, mod::hvg::control::hvg_serial_t* hvg_ptr, ui::log::Display_Log* log_ptr);
	void render(window_t* win);
	bool is_close(window_t* win);
}

#endif // _INCLUDE_WINDOW_H

#ifdef WINDOW_IMPLEMENTATION
#ifndef WINDOW_IMPLEMENTED
#define WINDOW_IMPLEMENTED
namespace ui {
	static void glfw_error_callback(int error, const char* description)
	{
		fprintf(stderr, "Glfw Error %d: %s\n", error, description);
	}

	bool init(window_t* win, int x, int y, int w, int h, array<float, 4> background_color, const char* title, std::function<bool(ui::window_t*, mod::hvg::control::hvg_serial_t*, ui::log::Display_Log*)> function_ptr)
	{
		glfwSetErrorCallback(glfw_error_callback);
		win->renders.push_back(function_ptr);
		/*	function_ptr = nullptr;*/
		win->w = w;
		win->h = h;
		win->clear_color = ImVec4(background_color.at(0), background_color.at(1), background_color.at(2), background_color.at(3));
		strcpy(win->title, title);
		if (!glfwInit()) {
			return false;
		}
		// Decide GL+GLSL versions
#ifdef __APPLE__
	// GL 3.2 + GLSL 150
		const char* glsl_version = "#version 150";
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
	// GL 3.0 + GLSL 130
		const char* glsl_version = "#version 130";
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
		//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
		//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif
		win->wnd = glfwCreateWindow(win->w, win->h, win->title, NULL, NULL);
		if (win->wnd == NULL) {
			return false;
		}
		glfwMakeContextCurrent(win->wnd);
		glfwSwapInterval(1);
		// Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
		bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
		bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
		bool err = gladLoadGL() == 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD2)
		bool err = gladLoadGL(glfwGetProcAddress) == 0; // glad2 recommend using the windowing library loader instead of the (optionally) bundled one.
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
		bool err = false;
		glbinding::Binding::initialize();
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
		bool err = false;
		glbinding::initialize([](const char* name) { return (glbinding::ProcAddress)glfwGetProcAddress(name); });
#else
		bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
		if (err)
		{
			fprintf(stderr, "Failed to initialize OpenGL loader!\n");
			return 1;
		}
		//Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		ImGui::StyleColorsClassic();
		ImGui_ImplGlfw_InitForOpenGL(win->wnd, true);
		ImGui_ImplOpenGL3_Init(glsl_version);
		return true;
	}
	void new_frame(window_t* win)
	{
		glfwPollEvents();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}
	void draw(window_t* win, mod::hvg::control::hvg_serial_t* hvg_ptr, ui::log::Display_Log* log_ptr)
	{
		std::function<bool(ui::window_t*, mod::hvg::control::hvg_serial_t*, ui::log::Display_Log*)>& fun_ptr = win->renders.front();
		fun_ptr(win, hvg_ptr, log_ptr);
		//std::list<std::function<bool(ui::window_t*)>>::iterator it;
		//for (it = win->renders.begin(); it != win->renders.end(); it++); {
		//	/**(it(win));*/
		//	//(*it)(win);
		//	/*std::function<bool(ui::window_t*)> fun_ptr = (*it);
		//	fun_ptr(win);*/
		//	*it;

		//}
	}
	//void draw(window_t* win)
	//{

	//}
	void render(window_t* win)
	{
		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(win->wnd, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(win->clear_color.x, win->clear_color.y, win->clear_color.z, win->clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(win->wnd);
	}
	bool is_close(window_t* win)
	{
		if (glfwWindowShouldClose(win->wnd)) {
			return true;
		}
		return false;
	}
	void clean_up(window_t* win)
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		glfwDestroyWindow(win->wnd);
		glfwTerminate();
	}

}
#endif // !WINDOW_IMPLEMENTED
#endif // WINDOW_IMPLEMENTATION
