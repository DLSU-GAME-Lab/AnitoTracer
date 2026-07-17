// AnitoTracer_Rebuild.cpp : Defines the entry point for the application.
//

#include "AnitoTracer_Rebuild.h"

#if BX_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32  // Exposes Windows-specific API features
#include <GLFW/glfw3native.h>     // Provides the real glfwGetWin32Window function
#endif

int main()
{
	std::cout << "Hello CMake." << std::endl;
	// Initialize GLFW
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return -1;
	}

	// Configure GLFW for bgfx
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// Create window
	GLFWwindow* window = glfwCreateWindow(1280, 720, "AnitoTracer - bgfx + GLFW", nullptr, nullptr);
	if (!window)
	{
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	// Get native window handle for bgfx
	bgfx::PlatformData pd;
	memset(&pd, 0, sizeof(pd));

#if BX_PLATFORM_WINDOWS
	pd.nwh = glfwGetWin32Window(window);
#endif

	// Initialize bgfx
	bgfx::setPlatformData(pd);
	bgfx::Init init;
	init.type = bgfx::RendererType::Vulkan;
	init.platformData = pd;                // Pass your GLFW window handle
	init.resolution.width = 1280;
	init.resolution.height = 720;
	init.resolution.reset = BGFX_RESET_VSYNC;

	// Initialize bgfx with settings
	if (!bgfx::init(init))
	{
		std::cerr << "Failed to initialize bgfx engine" << std::endl;
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	// Enable debug text
	bgfx::setDebug(BGFX_DEBUG_TEXT);

	std::cout << "bgfx and GLFW initialized successfully!" << std::endl;

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		// Get window size
		int width, height;
		glfwGetWindowSize(window, &width, &height);

		// Reset if window was resized
		if (width != 1280 || height != 720)
		{
			bgfx::reset(width, height, BGFX_RESET_VSYNC);
		}

		// Clear background
		bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
		bgfx::setViewRect(0, 0, 0, uint16_t(width), uint16_t(height));
		bgfx::touch(0);

		// Debug stats and text
		bgfx::dbgTextClear();
		bgfx::dbgTextPrintf(0, 0, 0x4f, "AnitoTracer - bgfx Renderer");
		bgfx::dbgTextPrintf(0, 1, 0x0f, "Resolution: %d x %d", width, height);
		bgfx::dbgTextPrintf(0, 2, 0x0f, "Press ESC to exit");

		// Frame submission
		bgfx::frame();

		// Process input
		glfwPollEvents();
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, true);
		}
	}

	// Cleanup
	bgfx::shutdown();
	glfwDestroyWindow(window);
	glfwTerminate();

	std::cout << "Cleanup complete. Exiting." << std::endl;
	return 0;
}
