#include "GLUtils/ShaderProgram.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

namespace
{
	// not really necessary
	bool compileShaderSource(const GLuint & programID, GLenum shaderType, const char * shaderPath)
	{
		// ensure ifstream objects can throw exceptions:
		std::ifstream shaderFile;
		shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		std::stringstream shaderStream;
		try
		{
			shaderFile.open(shaderPath);
			shaderStream << shaderFile.rdbuf();
			shaderFile.close();
		}
		catch (const std::ifstream::failure & e)
		{
			shaderFile.close();
			std::cout << "Error: Whilst reading shader file " << shaderPath << "\n";
			return false; // unsuccessful
		}

		const std::string streamStr = shaderStream.str(); // ugh
		const char * sourceStr = streamStr.c_str(); // ugh

		const GLuint shader = glCreateShader(shaderType);
		glShaderSource(shader, 1, &sourceStr, NULL);
		glCompileShader(shader);

		int shaderSuccess;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderSuccess);
		if (!shaderSuccess)
		{
			char log[512];
			glGetShaderInfoLog(shader, 512, NULL, log);
			std::cout << "Error: compiling shader component of type " << shaderType << ": " << log << "\n";
			glDeleteShader(shader);
			return false; // unsuccessful
		}

		// attach the shader to the shader program, and then delete it,
		// since it will be kept alive by the program
		glAttachShader(programID, shader);
		glDeleteShader(shader);
		return true;
	}

	void cacheUniformLocations(const GLuint & programID, std::unordered_map<std::string, GLint> & cache)
	{
		GLint maxUniformNameLen, numUniforms;
		glGetProgramiv(programID, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxUniformNameLen );
		glGetProgramiv(programID, GL_ACTIVE_UNIFORMS, &numUniforms );

		GLint read, size;
		GLenum type;

		std::vector<GLchar> uniformName(maxUniformNameLen, 0);
		for ( GLint i = 0; i < numUniforms; ++ i )
		{
			glGetActiveUniform(programID, i, maxUniformNameLen, &read, &size, &type, uniformName.data());
			cache[uniformName.data()] = glGetUniformLocation(programID, uniformName.data());
		}
	}
}

using namespace GLUtils;

ShaderProgram::ShaderProgram(const std::list<ShaderComponent> & components) : m_shaderProgramID(glCreateProgram())
{
	for (const ShaderComponent & component : components)
	{
		if (!compileShaderSource(m_shaderProgramID, component.type, component.source))
		{
			// break here, don't bother linking?
			std::cout<<"Error: compiling shader: "<<component.source<<"\n";
			return;
		}
	}

	// link the shaders to the program and
	glLinkProgram(m_shaderProgramID);

	int programSuccess;
	glGetProgramiv(m_shaderProgramID, GL_LINK_STATUS, &programSuccess);
	if(!programSuccess)
	{
		char log[512];
		glGetProgramInfoLog(m_shaderProgramID, 512, NULL, log);
		std::cout << "Error: shader program did not link: " << log << "\n";
		return;
	}

	// if linking was successful, we can cache all of the uniform locations
	cacheUniformLocations(m_shaderProgramID, m_uniformLocationCache);
}

GLint ShaderProgram::getUniformLocation(const char * uniformName)
{
	// TODO: Check if shader is valid?

	std::unordered_map<std::string, GLint>::const_iterator it = m_uniformLocationCache.find(uniformName);
	if (it != m_uniformLocationCache.end())
	{
		return it->second;
	}

	const GLint location = glGetUniformLocation(m_shaderProgramID, uniformName);
	m_uniformLocationCache[uniformName] = location;
	return location;
}
