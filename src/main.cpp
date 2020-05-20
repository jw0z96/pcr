#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

#include <algorithm>
#include <random>

#include <GL/glew.h> // load glew before SDL_opengl

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl.h>
#include <imgui/imgui_impl_opengl3.h>

#include <tinyply/tinyply.h>
#include "ply_utils.h"

#include "OrbitalCamera.h"

#include "GLUtils/Timer.h"
#include "GLUtils/Buffer.h"
#include "GLUtils/ShaderProgram.h"

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 1024

int main(int argc, char *argv[])
{
	std::cout<<"Starting PointCloudRendering\n";

	// default to bunny, otherwise get filepath from command line
	const std::string filepath = (argc > 1) ? argv[1] : "res/bunny.ply";
	std::cout<<"using file "<<filepath<<"\n";

	// initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		std::cout<<"SDL cannot init with error "<<SDL_GetError()<<"\n";
		return 1;
	}

	// set opengl version to use in this program, 4.2 core profile (until I find a deprecated function to use)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3); // :)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	// use multisampling?
	// SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	// SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

	// create window
	SDL_Window* window = SDL_CreateWindow(
		"OpenGL",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		SCREEN_WIDTH,
		SCREEN_HEIGHT,
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
	);

	if (!window)
	{
		std::cout<<"Couldn't create window, error: "<<SDL_GetError()<<"\n";
		return 1;
	}

	// create OpenGL context
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	if (!gl_context)
	{
		std::cout<<"Couldn't create OpenGL context, error: "<<SDL_GetError()<<"\n";
		return 1;
	}

	// initialize GLEW
	glewExperimental = GL_TRUE;
	GLenum glewError = glewInit();
	if (glewError != GLEW_OK)
	{
		std::cout<<"Error initializing GLEW:"<<glewGetErrorString(glewError)<<"\n";
		return 1;
	}

	// disable vsync
	SDL_GL_SetSwapInterval(0);

	// Init imgui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer bindings
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init("#version 420"); // glsl version

	// Scope to ensure ShaderProgram and GLBuffer destructors are called before window/context destructors
	{
		// set the viewport dimensions
		glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

		// glPointSize(10.0f);
		glEnable(GL_PROGRAM_POINT_SIZE);

		// create ID buffer fbo
		GLuint idFBO;
		glGenFramebuffers(1, &idFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, idFBO);
		glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT); // again?

		// create integer id texture
		GLuint idTexture;
		glGenTextures(1, &idTexture);
		// glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, idTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RED_INTEGER, GL_INT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		// glBindTexture(GL_TEXTURE_2D, 0);
		// attach it to FBO
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, idTexture, 0);

		// create depth attachment, because we can't enable depth testing otherwise
		GLuint depthTexture;
		glGenTextures(1, &depthTexture);
		// glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		// glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		// glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

		// std::cout<<"gl error:"<<glGetError()<<"\n";

		// tell OpenGL which attachments we'll use (of this framebuffer) for rendering
		unsigned int attachments[] = { GL_COLOR_ATTACHMENT0 }; // , GL_DEPTH_ATTACHMENT }; // we don't need to list the depth attachment
		glDrawBuffers(1, attachments);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cout << "Error: Framebuffer is not complete!\n";
			return 1;
		}

		// Create visibility compute shader
		GLUtils::ShaderProgram visComputeShader({
			{GL_COMPUTE_SHADER, "shaders/visibility_comp.glsl"}
		});

		// int seedLoc = visComputeShader.getUniformLocation("seed");

		// Create points shader
		GLUtils::ShaderProgram pointsShader({
			{GL_VERTEX_SHADER, "shaders/points_vert.glsl"},
			{GL_FRAGMENT_SHADER, "shaders/points_frag.glsl"}
		});

		// Create output shader
		GLUtils::ShaderProgram outputShader({
			{GL_VERTEX_SHADER, "shaders/screenspace_vert.glsl"},
			{GL_FRAGMENT_SHADER, "shaders/output_frag.glsl"}
		});

		// use texture unit 0 for the ID texture
		outputShader.use();
		glUniform1i(outputShader.getUniformLocation("idTexture"), 0);
		// use texture unit 1 for the depth texture
		glUniform1i(outputShader.getUniformLocation("depthTexture"), 1);
		// use texture unit 2 for the colour texture
		glUniform1i(outputShader.getUniformLocation("colTexture"), 2);
		// // use texture unit 3 for the colour texture
		// glUniform1i(outputShader.getUniformLocation("visTexture"), 3);

		// read the vertex positions and colours from the ply file
		std::shared_ptr<tinyply::PlyData> vert_pos, vert_col;
		ply_utils::read_ply_file(filepath, vert_pos, vert_col);
		unsigned int num_verts = vert_pos ? vert_pos->count : 0;
		std::cout<<"num_verts: "<<num_verts<<"\n";
		// we want 'num_verts' bits to be allocated for the visibility buffer, but this has to be allocated in bytes
		const size_t numVertsBytes = ceil(num_verts / 8.0f); // 8 bits per byte
		std::cout<<"num bytes: "<<numVertsBytes<<"\n";

		// we need to create a VAO and VBOs for the point cloud data
		unsigned int verts_vao, colTex;

		const GLUtils::Buffer pointsBuffer, colBuffer, visBuffer, elementBuffer;
		{
			// we have to generate and bind a VAO for the whole lot
			glGenVertexArrays(1, &verts_vao);
			glBindVertexArray(verts_vao);

			// generate buffers for verts, set the VAO attributes
			pointsBuffer.bindAs(GL_ARRAY_BUFFER);
			glBufferData(GL_ARRAY_BUFFER, vert_pos->buffer.size_bytes(), vert_pos->buffer.get(), GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

			// generate buffers for colours
			colBuffer.bindAs(GL_TEXTURE_BUFFER);
			glBufferData(GL_TEXTURE_BUFFER, vert_col->buffer.size_bytes(), vert_col->buffer.get(), GL_STATIC_DRAW);
			// map it to a texture
			glGenTextures(1, &colTex);
			glBindTexture(GL_TEXTURE_BUFFER, colTex);
			colBuffer.attachToTextureBuffer(GL_R8UI);

			// glActiveTexture(GL_TEXTURE1);
			// glBindTexture(GL_TEXTURE_BUFFER, colTex);
			// glTexBuffer(GL_TEXTURE_BUFFER, GL_R8UI, colBuffer);
			// glBindBuffer(GL_TEXTURE_BUFFER, 0);
			// optionally, enable it as a vertex attribute
			// glBindBuffer(GL_ARRAY_BUFFER, colBuffer);
			// glEnableVertexAttribArray(1);
			// glVertexAttribIPointer(1, 3, GL_UNSIGNED_BYTE, 3 * sizeof(unsigned char), (void*)0); // sized type

			// generate an SSBO for the visibility
			visBuffer.bindAs(GL_SHADER_STORAGE_BUFFER);
			glBufferData(GL_SHADER_STORAGE_BUFFER, numVertsBytes, NULL, GL_DYNAMIC_COPY);
			visBuffer.bindAsIndexed(GL_SHADER_STORAGE_BUFFER, 0);

			// Generate an element buffer for the point indices to redraw
			elementBuffer.bindAs(GL_ELEMENT_ARRAY_BUFFER);
			elementBuffer.bindAs(GL_SHADER_STORAGE_BUFFER);

			// give it enough elements for a full draw
			glBufferData(GL_SHADER_STORAGE_BUFFER, num_verts * sizeof(GLuint), NULL, GL_DYNAMIC_COPY);
			elementBuffer.bindAsIndexed(GL_SHADER_STORAGE_BUFFER, 1);

			std::cout << "gl error: " << glGetError() << "\n"; // TODO: A proper macro for glErrors
		}

		// Camera variables
		OrbitalCamera camera;
		pointsShader.use();
		glUniformMatrix4fv(pointsShader.getUniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(camera.getProjection()));

		// const GLint viewLoc = pointsShader.getUniformLocation("view");

		// roughly center the richmond-azaelias.ply model
		const glm::mat4 model = glm::translate(
			glm::rotate(glm::mat4(1.0), 3.14159f/2.0f, glm::vec3(-1.0f, 0.0f, 0.0f)),
			glm::vec3(0.0f, 0.0f, -5.0f)
		);

		glUniformMatrix4fv(pointsShader.getUniformLocation("model"), 1, GL_FALSE, glm::value_ptr(model));

		// const float cameraRadius = 20.0f;
		// glm::mat4 view;

		// count fps
		unsigned int fps_lasttime = SDL_GetTicks(); //the last recorded time.
		unsigned int fps_current = 0; //the current FPS.
		unsigned int fps_frames = 0; //frames passed since the last recorded fps.

		// bind the id texture to unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, idTexture);
		// bind the id texture to unit 1
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		// texture buffer for the colour to unit 2
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_BUFFER, colTex);

		// create an atomic counter, used to when getting the number of visible fragments
		// GLuint counterBuffer;
		const GLUtils::Buffer counterBuffer;
		GLuint counterValue = 0;
		// glGenBuffers(1, &counterBuffer);
		// glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counterBuffer);
		counterBuffer.bindAs(GL_ATOMIC_COUNTER_BUFFER);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &counterValue, GL_DYNAMIC_DRAW);
		// glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, counterBuffer);
		counterBuffer.bindAsIndexed(GL_ATOMIC_COUNTER_BUFFER, 0);

		// determine workgroup count
		int work_grp_cnt;
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt);
		int work_grp_size;
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size);
		const size_t dispatchCount = ceil(numVertsBytes / 4);

		std::cout<<"work_grp_cnt: "<<work_grp_cnt<<"\n";
		std::cout<<"work_grp_size: "<<work_grp_size<<"\n";
		std::cout<<"dispatchCount: "<<dispatchCount<<"\n";

		const GLuint zero = 0;
		unsigned int lastDraw = 0;
		int fillRate = 10000; // add n random elements each frame
		const unsigned int maxFillRate = std::min(static_cast<unsigned int>(100000), num_verts); // 100k reasonable maximum?
		bool doProgressive = false;

		bool quit = false;
		SDL_Event event;
		while (!quit)
		{
			GLUtils::startTimer(frameTimer);

			// event handling
			{

				while (SDL_PollEvent(&event) != 0)
				{
					ImGui_ImplSDL2_ProcessEvent(&event);
					camera.processInput(event);

					switch (event.type)
					{
						// exit if the window is closed
						case SDL_QUIT:
							quit = true;
							break;
						/*
						// check for keypresses
						case SDL_KEYDOWN:
							// if we want to recompile shaders? (replace the shader objects)
							if (event.key.keysym.sym == SDLK_r)
							{
							}
							break;
						*/
						default:
							break;
					}
				}

				// calculate framerate
				fps_frames++;
				const unsigned int & ticks = SDL_GetTicks();
				if (fps_lasttime < ticks - 1000) // 1000 is the interval? (1000ms, 1sec)
				{
					fps_lasttime = ticks;
					fps_current = fps_frames;
					fps_frames = 0;
				}

			}


			{
				GLUtils::scopedTimer(idPassTimer);

				// ID pass
				glBindFramebuffer(GL_FRAMEBUFFER, idFBO);
				glEnable(GL_DEPTH_TEST);
				// glDepthMask(GL_TRUE);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				pointsShader.use();

				// updated uniform with new view matrix
				glUniformMatrix4fv(pointsShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(camera.getView()));

				if (doProgressive)
				{
					size_t numVisible = 0;
					{
						GLUtils::scopedTimer(indexComputeTimer);

					// prepare our element buffer for drawing
					// prepare the 'count' argument for glDrawElements
					#define EBO_GPU true
					#if EBO_GPU
						{
							{
								GLUtils::scopedTimer(indexComputeSetupTimer);
							// use a compute shader to count the elements in the visibility buffer
								visComputeShader.use();
							// this should already be set
								visBuffer.bindAsIndexed(GL_SHADER_STORAGE_BUFFER, 0);
								elementBuffer.bindAsIndexed(GL_SHADER_STORAGE_BUFFER, 1);
							}
						// since we can only access the buffer as unsigned ints, we need to dispatch ceil(numVertsBytes / sizeof(uint)) shader invocations
							{
								GLUtils::scopedTimer(indexComputeDispatchTimer);
								glDispatchCompute(dispatchCount, 1, 1);
							}
							{
								GLUtils::scopedTimer(indexCounterReadTimer);
							// get the atomic counter value
								glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &numVisible);
							}
							{
								GLUtils::scopedTimer(indexCounterResetTimer);
							// reset the counter value
								glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &zero, GL_DYNAMIC_DRAW);
							// TODO: Filling random undrawn indices
							}
						}
					#else // CPU
						// count our visibility buffer
						std::vector<GLubyte> buf(numVertsBytes, 0);
						visBuffer.bindAs(GL_SHADER_STORAGE_BUFFER);
						glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, numVertsBytes, buf.data());
						std::vector<GLuint> visibleIndices, fillIndices;
						visibleIndices.reserve(num_verts);
						fillIndices.reserve(num_verts);
						/*
						for (int i = 0; i < buf.size(); ++i)
						{
							GLubyte e = buf[i];
							while (e)
							{
								++numVisible;
								e &= e - 1;
							}
						}
						*/
						size_t pointIndex = 0;
						for (size_t i = 0; i < numVertsBytes; ++i)
						{
							GLubyte e = buf[i];
							// 8 bits per ubyte, so we bit shift
							for (int j = 0; j < 8; ++j, ++pointIndex)
							{
								if (e & 1)
								{
									visibleIndices.push_back(pointIndex);
								}
								else
								{
									fillIndices.push_back(pointIndex);
								}

								e >>= 1;
							}
						}

						numVisible = visibleIndices.size();

						// if we vertices to shuffle in, shuffle in some of the invisible points
						if (!fillIndices.empty())
						{
							size_t fillCount = std::min(static_cast<size_t>(fillRate), fillIndices.size());
							std::shuffle(std::begin(fillIndices), std::end(fillIndices), rng);
							for (size_t i = 0; i < fillCount; ++i)
							{
								visibleIndices.push_back(fillIndices[i]);
							}
							numVisible += fillCount;
						}

						// elementBuffer.bindAs(GL_ELEMENT_ARRAY_BUFFER);
						glBufferData(GL_ELEMENT_ARRAY_BUFFER, numVisible * sizeof(GLuint), &visibleIndices[0], GL_DYNAMIC_COPY);

						// clear our visibility buffer
						glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
					#endif
					}
					{
						// draw points
						GLUtils::scopedTimer(pointsDrawTimer);
						pointsShader.use();
						// glBindVertexArray(verts_vao);
						// elementBuffer.bindAs(GL_ELEMENT_ARRAY_BUFFER);
						glDrawElements(GL_POINTS, numVisible, GL_UNSIGNED_INT, 0);
						lastDraw = numVisible;
					}
				}
				else
				{
					// draw points
					GLUtils::scopedTimer(pointsDrawTimer);
					pointsShader.use();
					// glBindVertexArray(verts_vao);
					glDrawArrays(GL_POINTS, 0, num_verts);
					lastDraw = num_verts;
				}

			}

			{
				GLUtils::scopedTimer(outputPassTimer);

				// screenspace output pass
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glClear(GL_COLOR_BUFFER_BIT);
				glDisable(GL_DEPTH_TEST);
				outputShader.use();
				// The vertex shader will create a screen space quad, so no need to bind a different VAO & VBO
				glDrawArrays(GL_TRIANGLES, 0, 6);
			}

			// draw the UI elements
			{

				// Start the ImGui frame
				ImGui_ImplOpenGL3_NewFrame();
				ImGui_ImplSDL2_NewFrame(window);
				ImGui::NewFrame();

				// stats window
				ImGui::Begin("Controls");
				ImGui::Checkbox("Progressive Render", &doProgressive);

				if (doProgressive)
				{
					ImGui::Text("Fill Rate (per frame):");
					ImGui::SliderInt("", &fillRate, 0, maxFillRate);
				}

				ImGui::Separator();
				ImGui::Text("Framerate: %u fps", fps_current);
				ImGui::Text("Drawing %u / %u points (%.2f%%)", lastDraw, num_verts, (lastDraw * 100.0f / num_verts));

				ImGui::Separator();
				ImGui::Text("Frame time: %.1f ms", GLUtils::getElapsed(frameTimer));
				ImGui::Text("	ID Pass time: %.1f ms", GLUtils::getElapsed(idPassTimer));
				if (doProgressive)
				{
					ImGui::Text("		Index Compute time: %.1f ms", GLUtils::getElapsed(indexComputeTimer));
					ImGui::Text("			Index Compute Setup time: %.1f ms", GLUtils::getElapsed(indexComputeSetupTimer));
					ImGui::Text("			Index Compute Dispatch time: %.1f ms", GLUtils::getElapsed(indexComputeDispatchTimer));
					ImGui::Text("			Index Counter Read time: %.1f ms", GLUtils::getElapsed(indexCounterReadTimer));
					ImGui::Text("			Index Counter Reset time: %.1f ms", GLUtils::getElapsed(indexCounterResetTimer));
				}
				ImGui::Text("		Points Draw time: %.1f ms", GLUtils::getElapsed(pointsDrawTimer));
				ImGui::Text("	Output Pass time: %.1f ms", GLUtils::getElapsed(outputPassTimer));
				ImGui::End();

				// Rendering
				ImGui::Render();
				ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			}

			// swap window to update opengl
			{
				SDL_GL_SwapWindow(window);
			}

			GLUtils::endTimer(frameTimer);
		}

		glDeleteFramebuffers(1, &idFBO);

		glDeleteVertexArrays(1, &verts_vao);

		glDeleteTextures(1, &idTexture);
		glDeleteTextures(1, &depthTexture);
		glDeleteTextures(1, &colTex);
	}

	// Cleanup
	GLUtils::clearTimers();


	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	std::cout << "bye!\n";
	return 0;
}