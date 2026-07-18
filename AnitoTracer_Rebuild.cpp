// AnitoTracer_Rebuild.cpp : Defines the entry point for the application.
//

#include "AnitoTracer_Rebuild.h"

#if BX_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32  // Exposes Windows-specific API features
#include <GLFW/glfw3native.h>     // Provides the real glfwGetWin32Window function
#endif

#include "src/Rendering/Shaders/SimpleShader.hpp"

struct PosColorVertex {
	float x, y, z;      // Position
	float r, g, b, a;   // Color
};

// 2. Define the geometry data for a simple center triangle
static PosColorVertex s_triangleVertices[] = {
	{  0.0f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f }, // Top Vertex (Red)
	{  0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f, 1.0f }, // Bottom Right Vertex (Green)
	{ -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f }  // Bottom Left Vertex (Blue)
};

static const uint16_t s_triangleIndices[] = {
	0, 1, 2
};

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

	SimpleShader shader;

	bgfx::VertexBufferHandle vbh = bgfx::createVertexBuffer(
		bgfx::makeRef(s_triangleVertices, sizeof(s_triangleVertices)),
		shader.GetLayout()
	);

	bgfx::IndexBufferHandle ibh = bgfx::createIndexBuffer(
		bgfx::makeRef(s_triangleIndices, sizeof(s_triangleIndices))
	);

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		// Get window size
		int width, height;
		glfwGetWindowSize(window, &width, &height);

		// Handle resize tracking dynamically
		static int currentWidth = 1280;
		static int currentHeight = 720;
		if (width != currentWidth || height != currentHeight)
		{
			currentWidth = width;
			currentHeight = height;
			bgfx::reset(uint32_t(width), uint32_t(height), BGFX_RESET_VSYNC);
		}

		// 1. Configure View 0 properties FIRST
		bgfx::setViewRect(0, 0, 0, uint16_t(width), uint16_t(height));
		bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);

		// 2. Touch View 0 to ensure it clears even if nothing throws drawing commands
		bgfx::touch(0);

		// 3. Bind geometry streams
		bgfx::setVertexBuffer(0, vbh);
		bgfx::setIndexBuffer(ibh);

		// 4. Set state explicit override: Disable culling to isolate if it's a winding order issue
		uint64_t state = BGFX_STATE_WRITE_RGB
			| BGFX_STATE_WRITE_A
			| BGFX_STATE_WRITE_Z
			| BGFX_STATE_DEPTH_TEST_LESS;
		bgfx::setState(state);

		// 5. Submit primitive payload to View 0
		shader.Submit(0);

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

	bgfx::destroy(vbh);
	bgfx::destroy(ibh);
	shader.Unload();

	// Cleanup
	bgfx::shutdown();
	glfwDestroyWindow(window);
	glfwTerminate();

	std::cout << "Cleanup complete. Exiting." << std::endl;
	return 0;
}
