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

#include <tinyply/tinyply.h>
#include "ply_utils.h"

#include "ShaderProgram.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 640

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
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
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

	{
		if (!window)
		{
			std::cout<<"Couldn't create window, error: "<<SDL_GetError()<<"\n";
			return 1;
		}

		// create OpenGL context
		if (!SDL_GL_CreateContext(window))
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
	}

	// disable vsync
	SDL_GL_SetSwapInterval(0);

	// set the viewport dimensions
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	// TODO: this only applies to the ID fbo??

	// enable depth testing...
	glEnable(GL_DEPTH_TEST);
	// glDepthMask(GL_FALSE);

	// 	glEnable(GL_BLEND);
	// 	glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	// 	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// 	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	// 	glBlendFunc(GL_SRC_ALPHA,GL_ONE);

	// glPointSize(10.0f);
	glEnable(GL_PROGRAM_POINT_SIZE);

	// glEnable(GL_ALPHA_TEST);
	// glAlphaFunc(GL_GREATER, 0.9);

	// create ID buffer fbo
	GLuint idFBO;
	glGenFramebuffers(1, &idFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, idFBO);
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT); // again?
	// glDepthMask(GL_TRUE);
	// glEnable(GL_DEPTH_TEST);

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

	// compile and use points shader
	ShaderProgram pointsShader;
	{
		pointsShader.addShaderSource(GL_VERTEX_SHADER, "shaders/points_vert.glsl");
		pointsShader.addShaderSource(GL_FRAGMENT_SHADER, "shaders/points_frag.glsl");
		if (!pointsShader.compile())
		{
			std::cout<<"Error: pointsShader did not compile!\n";
			return 1;
		}
	}

	// compile and use output shader
	ShaderProgram outputShader;
	{
		outputShader.addShaderSource(GL_VERTEX_SHADER, "shaders/screenspace_vert.glsl");
		outputShader.addShaderSource(GL_FRAGMENT_SHADER, "shaders/output_frag.glsl");
		if (!outputShader.compile())
		{
			std::cout<<"Error: outputShader did not compile!\n";
			return 1;
		}
		// use texture unit 0 for the ID texture
		outputShader.use();
		glUniform1i(outputShader.getUniformLocation("idTexture"), 0);
		// use texture unit 1 for the depth texture
		glUniform1i(outputShader.getUniformLocation("depthTexture"), 1);
		// use texture unit 2 for the colour texture
		glUniform1i(outputShader.getUniformLocation("colTexture"), 2);
		// // use texture unit 3 for the colour texture
		// glUniform1i(outputShader.getUniformLocation("visTexture"), 3);
	}

	// read the vertex positions and colours from the ply file
	std::shared_ptr<tinyply::PlyData> vert_pos, vert_col;
	ply_utils::read_ply_file(filepath, vert_pos, vert_col);
	unsigned int num_verts = vert_pos ? vert_pos->count : 0;
	std::cout<<"num_verts: "<<num_verts<<"\n";
	// we want 'num_verts' bits to be allocated for the visibility buffer, but this has to be allocated in bytes
	const size_t numVertsBytes = ceil(num_verts / 8.0f); // 8 bits per byte
	std::cout<<"num bytes: "<<numVertsBytes<<"\n";

	// we need to create a VAO and VBOs for the point cloud data
	unsigned int verts_vao, verts_vbo, colBuffer, colTex, visBuffer, elementBuffer;
	{
		// we have to generate and bind a VAO for the whole lot
		glGenVertexArrays(1, &verts_vao);
		glBindVertexArray(verts_vao);

		// generate buffers for verts, set the VAO attributes
		glGenBuffers(1, &verts_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, verts_vbo);
		glBufferData(GL_ARRAY_BUFFER, vert_pos->buffer.size_bytes(), vert_pos->buffer.get(), GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

		glBindVertexArray(0);

		// generate buffers for colours
		glGenBuffers(1, &colBuffer);
		glBindBuffer(GL_TEXTURE_BUFFER, colBuffer);
		glBufferData(GL_TEXTURE_BUFFER, vert_col->buffer.size_bytes(), vert_col->buffer.get(), GL_STATIC_DRAW);
		// map it to a texture
		glGenTextures(1, &colTex);
		glBindTexture(GL_TEXTURE_BUFFER, colTex);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_R8UI, colBuffer);

		// glActiveTexture(GL_TEXTURE1);
		// glBindTexture(GL_TEXTURE_BUFFER, colTex);
		// glTexBuffer(GL_TEXTURE_BUFFER, GL_R8UI, colBuffer);
		// glBindBuffer(GL_TEXTURE_BUFFER, 0);
		// optionally, enable it as a vertex attribute
		// glBindBuffer(GL_ARRAY_BUFFER, colBuffer);
		// glEnableVertexAttribArray(1);
		// glVertexAttribIPointer(1, 3, GL_UNSIGNED_BYTE, 3 * sizeof(unsigned char), (void*)0); // sized type

		// generate an SSBO for the visibility
		glGenBuffers(1, &visBuffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, visBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, numVertsBytes, NULL, GL_DYNAMIC_COPY);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, visBuffer);

		// Generate an element buffer for the point indices to redraw
		glGenBuffers(1, &elementBuffer);
		// glBindBuffer(GL_SHADER_STORAGE_BUFFER, elementBuffer);
		// glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, elementBuffer);

		std::cout<<"gl error:"<<glGetError()<<"\n";
	}

	// create an empty VAO for the screenspace quad, which will be created in a shader...
	GLuint emptyVAO;
	glGenVertexArrays(1, &emptyVAO);

	// Camera variables
	pointsShader.use();
		const glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 1.0f, 100.0f);
		GLint projectionLoc = pointsShader.getUniformLocation("projection");
		glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

		int viewLoc = pointsShader.getUniformLocation("view");

		/*
			glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f,  3.0f);
			glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
			glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f,  0.0f);

			glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

			const glm::mat4 model = glm::translate(
				glm::rotate(glm::mat4(1.0), 3.14159f, glm::vec3(1.0f, 0.0f, 0.0f)),
				glm::vec3(0.0f, 0.0f, -5.0f)
			);
		*/

		const glm::mat4 model = glm::translate(
			glm::rotate(glm::mat4(1.0), 3.14159f/2.0f, glm::vec3(-1.0f, 0.0f, 0.0f)),
			glm::vec3(0.0f, 0.0f, -5.0f)
		);
		// const glm::mat4 model = glm::rotate(glm::mat4(1.0), 3.14159f/2.0f, glm::vec3(-1.0f, 0.0f, 0.0f));
		glUniformMatrix4fv(pointsShader.getUniformLocation("model"), 1, GL_FALSE, glm::value_ptr(model));

		const float cameraRadius = 20.0f;
		glm::mat4 view;

	// count fps
	#define FPS_INTERVAL 1.0 // seconds.
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

	auto rng = std::default_random_engine {};

	size_t lastDraw = 0;
	const size_t fillRate = 10000; // add 100 random elements each frame
	const GLuint zero = 0;

	bool quit = false;
	SDL_Event event;
	while (!quit)
	{
		while (SDL_PollEvent(&event) != 0)
		{
			switch (event.type)
			{
				// exit if the window is closed
				case SDL_QUIT:
					quit = true;
					break;
				// check for keypresses
				case SDL_KEYDOWN:
					if (event.key.keysym.sym == SDLK_r)
					{
						// outputShader.compile();
						// pointsShader.compile();
						// pointsShader.use();
						// uhh
						// update the location of the 'view' uniform location
						// viewLoc = pointsShader.getUniformLocation("view");
						// update the values in the the projection and model uniforms
						// glUniformMatrix4fv(
						// 	pointsShader.getUniformLocation("projection"),
						// 	1,
						// 	GL_FALSE,
						// 	glm::value_ptr(projection)
						// );
						// glUniformMatrix4fv(
						// 	pointsShader.getUniformLocation("model"),
						// 	1,
						// 	GL_FALSE,
						// 	glm::value_ptr(model)
						// );
					}
					break;
				default:
					break;
			}
		}

		fps_frames++;
		const unsigned int & ticks = SDL_GetTicks();
		if (fps_lasttime < ticks - 1000) // 1000 is the interval? (1000ms, 1sec)
		{
			fps_lasttime = ticks;
			fps_current = fps_frames;
			fps_frames = 0;
			// std::cout<<"fps: "<<fps_current<<"\n";
		}

		// std::cout<<"gl error:"<<glGetError()<<"\n";

		// ID pass
		glBindFramebuffer(GL_FRAMEBUFFER, idFBO);
		glEnable(GL_DEPTH_TEST);
		// glDepthMask(GL_TRUE);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		pointsShader.use();

		// updated uniform with new view matrix
		const float time = ticks * 0.0001f;
		const float camX = sin(time) * cameraRadius;
		const float camZ = cos(time) * cameraRadius;
		view = glm::lookAt(glm::vec3(camX, 0.0, camZ), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

		// prepare our element buffer for drawing
		size_t numVisible = 0;
		{
			// glMemoryBarrier(GL_ALL_BARRIER_BITS);
			// count our visibility buffer
			std::vector<GLubyte> buf(numVertsBytes, 0);
			// glBindBuffer(GL_SHADER_STORAGE_BUFFER, visBuffer);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, numVertsBytes, buf.data());

			std::vector<GLuint> visibleIndices, fillIndices; //(num_verts, 0);
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

			std::cout<<numVisible<<" visible points ("<<(float(numVisible) / float(num_verts)) * 100.0f<<"% of total) "<<(float(numVisible) / float(lastDraw)) * 100.0f<<"% of last draw) fps: "<<fps_current<<"\n";

			// if we vertices to shuffle in, shuffle in some of the invisible points
			if (!fillIndices.empty())
			{
				size_t fillCount = std::min(fillRate, fillIndices.size());
				std::shuffle(std::begin(fillIndices), std::end(fillIndices), rng);
				for (size_t i = 0; i < fillCount; ++i)
				{
					visibleIndices.push_back(fillIndices[i]);
				}
				numVisible += fillCount;
			}

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, numVisible * sizeof(GLuint), &visibleIndices[0], GL_DYNAMIC_COPY);

			// clear our visibility buffer
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, visBuffer);
			glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
			// glMemoryBarrier(GL_ALL_BARRIER_BITS);
		}

		// draw points
		glBindVertexArray(verts_vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
		glDrawElements(GL_POINTS, numVisible, GL_UNSIGNED_INT, 0);
		// glDrawArrays(GL_POINTS, 0, 5);
		lastDraw = numVisible;

		// screenspace output pass
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);
		outputShader.use();

		// screenspace pass
		glBindVertexArray(emptyVAO);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		// swap window to update opengl
		SDL_GL_SwapWindow(window);
	}

	glDeleteFramebuffers(1, &idFBO);

	SDL_DestroyWindow(window);
	SDL_Quit();

	std::cout<<"bye!\n";
	return 0;
}