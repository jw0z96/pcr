#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

#include <GL/glew.h> // load glew before SDL_opengl

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <tinyply/tinyply.h>
#include "ply_utils.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

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
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
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

	// disable vsync
	SDL_GL_SetSwapInterval(0);

	// set the viewport dimensions
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	// enable depth testing...
	glEnable(GL_DEPTH_TEST);
	// glDepthMask(GL_FALSE);

	//
	// glEnable(GL_BLEND);
	// glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	// glBlendFunc(GL_SRC_ALPHA,GL_ONE);

	//
	// glPointSize(10.0f);
	glEnable(GL_PROGRAM_POINT_SIZE);

	// glEnable(GL_ALPHA_TEST);
	// glAlphaFunc(GL_GREATER, 0.9);

	// get the shader sources from file
	const std::string vertexPath = "shaders/points_vert.glsl";
	const std::string fragmentPath = "shaders/points_frag.glsl";

	std::string vertexCode, fragmentCode;
	std::ifstream vShaderFile, fShaderFile;

	// ensure ifstream objects can throw exceptions:
	vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try
	{
		// open files
		vShaderFile.open(vertexPath);
		fShaderFile.open(fragmentPath);
		std::stringstream vShaderStream, fShaderStream;
		// read file's buffer contents into streams
		vShaderStream << vShaderFile.rdbuf();
		fShaderStream << fShaderFile.rdbuf();
		// close file handlers
		vShaderFile.close();
		fShaderFile.close();
		// convert stream into string
		vertexCode = vShaderStream.str();
		fragmentCode = fShaderStream.str();
	}
	catch(std::ifstream::failure e)
	{
		std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
	}

	// compile and link shaders, store error status & log
	int success;
	char infoLog[512];

	// vertex shader
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	const char * vertexStr = vertexCode.c_str();
	glShaderSource(vertexShader, 1, &vertexStr, NULL);
	glCompileShader(vertexShader);

	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if(!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}

	// fragment shader
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	const char * fragmentStr = fragmentCode.c_str();
	glShaderSource(fragmentShader, 1, &fragmentStr, NULL);
	glCompileShader(fragmentShader);

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if(!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}

	// create shader program and link
	unsigned int shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	// if the shader program linked, delete the shaders?
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if(success)
	{
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
	}
	else
	{
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}

	// use the shader
	glUseProgram(shaderProgram);

	// read the vertex positions and colours from the ply file
	std::shared_ptr<tinyply::PlyData> vert_pos, vert_col;
	ply_utils::read_ply_file(filepath, vert_pos, vert_col);
	unsigned int num_verts = vert_pos ? vert_pos->count : 0;

	// we have to generate and bind a VAO for the whole lot
	unsigned int verts_vao;
	glGenVertexArrays(1, &verts_vao);
	glBindVertexArray(verts_vao);

	// generate buffers for verts, set the VAO attributes
	unsigned int verts_vbo;
	glGenBuffers(1, &verts_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, verts_vbo);
	glBufferData(GL_ARRAY_BUFFER, vert_pos->buffer.size_bytes(), vert_pos->buffer.get(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	// generate buffers for verts, set the VAO attributes
	unsigned int col_vbo;
	glGenBuffers(1, &col_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, col_vbo);
	glBufferData(GL_ARRAY_BUFFER, vert_col->buffer.size_bytes(), vert_col->buffer.get(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(1);
	glVertexAttribIPointer(1, 3, GL_UNSIGNED_BYTE, 3 * sizeof(unsigned char), (void*)0); // sized type

	// Camera variables
	const glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
	const int projectionLoc = glGetUniformLocation(shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	const int viewLoc = glGetUniformLocation(shaderProgram, "view");

	// glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f,  3.0f);
	// glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	// glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f,  0.0f);

	// glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

	// const glm::mat4 model = glm::translate(
	// 	glm::rotate(glm::mat4(1.0), 3.14159f, glm::vec3(1.0f, 0.0f, 0.0f)),
	// 	glm::vec3(0.0f, 0.0f, -5.0f)
	// );

	const glm::mat4 model = glm::translate(
		glm::rotate(glm::mat4(1.0), 3.14159f/2.0f, glm::vec3(-1.0f, 0.0f, 0.0f)),
		glm::vec3(0.0f, 0.0f, -5.0f)
	);
	// const glm::mat4 model = glm::rotate(glm::mat4(1.0), 3.14159f/2.0f, glm::vec3(-1.0f, 0.0f, 0.0f));

	const int modelLoc = glGetUniformLocation(shaderProgram, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));


	const float cameraRadius = 20.0f;
	glm::mat4 view;

	// count fps
	#define FPS_INTERVAL 1.0 // seconds.
	unsigned int fps_lasttime = SDL_GetTicks(); //the last recorded time.
	unsigned int fps_current = 0; //the current FPS.
	unsigned int fps_frames = 0; //frames passed since the last recorded fps.

	bool quit = false;
	SDL_Event sdlEvent;
	while (!quit)
	{
		// SDL_PollEvent(&sdlEvent);
		while (SDL_PollEvent(&sdlEvent) != 0)
		{
			if (sdlEvent.type == SDL_QUIT)
			{
				quit = true;
			}
		}

		fps_frames++;
		const unsigned int & ticks = SDL_GetTicks();
		if (fps_lasttime < ticks - 1000) // 1000 is the interval? (1000ms, 1sec)
		{
			fps_lasttime = ticks;
			fps_current = fps_frames;
			fps_frames = 0;
			std::cout<<"fps: "<<fps_current<<"\n";
		}

		// updated uniform with new view matrix
		const float time = ticks * 0.0001f;
		const float camX = sin(time) * cameraRadius;
		const float camZ = cos(time) * cameraRadius;
		view = glm::lookAt(glm::vec3(camX, 0.0, camZ), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

		// clear gl buffers
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// draw points
		// glDrawArrays(GL_POINTS, 0, num_verts);
		glDrawArrays(GL_POINTS, 0, num_verts);

		// swap window to update opengl
		SDL_GL_SwapWindow(window);
	}

	// std::cout<<"error: "<<glGetError()<<"\n";

	SDL_DestroyWindow(window);
	SDL_Quit();

	std::cout<<"bye!\n";
	return 0;
}