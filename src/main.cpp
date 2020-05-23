#include <iostream>

#include <GL/glew.h> // load glew before SDL_opengl

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl.h>
#include <imgui/imgui_impl_opengl3.h>

#include "PointCloudScene.h"

#include "GLUtils/Timer.h"

// window should process width/height changes as an event, which can be queried by FBO objects
constexpr unsigned int DEFAULT_SCREEN_WIDTH = 800;
constexpr unsigned int DEFAULT_SCREEN_HEIGHT = 600;

// These are all things that should be handled properly eventually
namespace SDL_GL_IMGUI_APP
{
	int SDLError(const char* msg)
	{
		std::cout << msg << SDL_GetError() << "\n";
		SDL_Quit();
		return EXIT_FAILURE;
	}

	void cleanup(SDL_Window* window, const SDL_GLContext& context, ImGuiContext* imgui)
	{
		GLUtils::clearTimers();

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext(imgui);

		SDL_GL_DeleteContext(context);
		SDL_DestroyWindow(window);

		SDL_Quit();
	}

	SDL_GLContext createGLContext(SDL_Window* window)
	{
		// set opengl version to use in this program, 4.2 core profile (until I find a deprecated function to
		// use)
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3); // :)
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

		// use multisampling?
		// SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		// SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
		return SDL_GL_CreateContext(window);
	}
} // namespace SDL_GL_IMGUI_APP

int main(int argc, char* argv[])
{
	std::cout << "Starting PointCloudRendering\n";

	// Get command line arguments, we should really bail / print a help message here
	const char* filepath = (argc > 1) ? argv[1] : "res/richmond-azaelias.ply";

	// initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		return SDL_GL_IMGUI_APP::SDLError("SDL cannot init with error: ");
	}

	// create window - TODO: RAII
	SDL_Window* window = SDL_CreateWindow(
		"OpenGL", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT,
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	if (!window)
	{
		return SDL_GL_IMGUI_APP::SDLError("SDL cannot create window, error: ");
	}

	// create OpenGL context - TODO: RAII
	const SDL_GLContext glContext = SDL_GL_IMGUI_APP::createGLContext(window);
	if (!glContext)
	{
		// kill the window first since we don't have RAII
		SDL_DestroyWindow(window);
		return SDL_GL_IMGUI_APP::SDLError("SDL cannot create OpenGL context, error: ");
	}

	// disable vsync in the glContext
	SDL_GL_SetSwapInterval(0);

	// initialize GLEW once we have a valid GL context - TODO: GLAD?
	glewExperimental = GL_TRUE;
	GLenum glewError = glewInit();
	if (glewError != GLEW_OK)
	{
		std::cout << "GLEW cannot init with error: " << glewGetErrorString(glewError) << "\n";
		// manually destroy SDL stuff
		SDL_GL_DeleteContext(glContext);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return EXIT_FAILURE;
	}

	// Init imgui
	IMGUI_CHECKVERSION();
	ImGuiContext* imGuiContext = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer bindings
	ImGui_ImplSDL2_InitForOpenGL(window, glContext);
	ImGui_ImplOpenGL3_Init("#version 420"); // glsl version

	// scope to ensure PointCloudScene dtor is called before cleanup
	{
		PointCloudScene scene;
		scene.loadPointCloud(filepath); // TODO: handle failure to load file?
		scene.setFramebufferParams(DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT);

		SDL_Event event;
		bool running = true;
		while (running)
		{
			// Event handling
			while (SDL_PollEvent(&event) != 0)
			{
				running = event.type != SDL_QUIT; // Exit if the window is closed
				scene.processEvent(event);
			}

			// Draw the scene
			scene.drawScene();

			// Start the ImGui frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplSDL2_NewFrame(window);
			ImGui::NewFrame();
			// Populate the ImGui frame with scene info
			scene.drawGUI();
			// Draw the ImGui frame
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			// swap window to update opengl
			SDL_GL_SwapWindow(window);
		}
	}

	SDL_GL_IMGUI_APP::cleanup(window, glContext, imGuiContext);
	return EXIT_SUCCESS;
}
