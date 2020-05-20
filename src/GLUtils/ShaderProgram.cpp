#include "GLUtils/ShaderProgram.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include <algorithm>
#include <functional>

namespace
{
	bool compileShaderSource(const GLuint& programID, GLenum shaderType, const char* shaderPath)
	{
		// ensure ifstream objects can throw exceptions:
		std::ifstream shaderFile;
		shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		std::string shaderStr;
		try
		{
			shaderFile.open(shaderPath);
			shaderStr.assign(
				(std::istreambuf_iterator<char>(shaderFile)), (std::istreambuf_iterator<char>()));
			shaderFile.close();
		}
		catch (const std::ifstream::failure& e)
		{
			shaderFile.close();
			std::cout << "Error: Whilst reading shader file " << shaderPath << "\n";
			return false; // unsuccessful
		}

		const GLuint shader = glCreateShader(shaderType);
		const char* shaderCStr = shaderStr.c_str(); // ugh
		glShaderSource(shader, 1, &shaderCStr, NULL);
		glCompileShader(shader);

		int shaderSuccess;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderSuccess);
		if (!shaderSuccess)
		{
			char log[512];
			glGetShaderInfoLog(shader, 512, NULL, log);
			std::cout << "Error: Failed to compile shader component of type " << shaderType << ": " << log << "\n";
			glDeleteShader(shader);
			return false; // unsuccessful
		}

		// attach the shader to the shader program, and then delete it,
		// since it will be kept alive by the program
		glAttachShader(programID, shader);
		glDeleteShader(shader);
		return true;
	}
} // namespace

using namespace GLUtils;

ShaderProgram::ShaderProgram(const std::list<ShaderComponent>& components) :
	m_shaderProgramID(glCreateProgram()), m_isValid(false)
{
	if (!std::all_of(components.begin(), components.end(), [this](const ShaderComponent& component) {
			return compileShaderSource(m_shaderProgramID, component.type, component.source);
		}))
	{
		// break here, don't bother linking?
		std::cout << "Error: Failed whilst compiling shaders, will not attempt linking\n";
		return;
	}

	// link the shaders to the program and
	glLinkProgram(m_shaderProgramID);

	int programSuccess;
	glGetProgramiv(m_shaderProgramID, GL_LINK_STATUS, &programSuccess);
	if (!programSuccess)
	{
		char log[512];
		glGetProgramInfoLog(m_shaderProgramID, 512, NULL, log);
		std::cout << "Error: shader program did not link: " << log << "\n";
		return;
	}

	m_isValid = true;

	// if linking was successful, we can cache all of the uniform locations
	GLint maxUniformNameLen, numUniforms;
	glGetProgramiv(m_shaderProgramID, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxUniformNameLen);
	glGetProgramiv(m_shaderProgramID, GL_ACTIVE_UNIFORMS, &numUniforms);

	GLint read, size;
	GLenum type;

	std::vector<GLchar> uniformName(maxUniformNameLen, 0);
	for (GLint i = 0; i < numUniforms; ++i)
	{
		glGetActiveUniform(m_shaderProgramID, i, maxUniformNameLen, &read, &size, &type, uniformName.data());
		/*
		std::cout << "Shader[" << m_shaderProgramID << "] uniform[" << i << "] name["
				  << std::string(uniformName.data()) << "] location["
				  << glGetUniformLocation(m_shaderProgramID, uniformName.data()) << "]\n";
		*/
		const GLchar* uniformNameStr = uniformName.data();
		m_uniformLocationCache[uniformNameStr] = glGetUniformLocation(m_shaderProgramID, uniformNameStr);
	}
}

void ShaderProgram::use() const
{
	if (!m_isValid)
	{
		// TODO: Throw something here...
		return;
	}

	glUseProgram(m_shaderProgramID);
}

GLint ShaderProgram::getUniformLocation(const char* uniformName) const
{
	if (!m_isValid)
	{
		// TODO: Throw something here...
		return -1; // -1 is the same as invalid location
	}

	// m_uniformLocationCache should have found all valid locations, so just return -1 invalid location) if
	// not found
	std::unordered_map<std::string, GLint>::const_iterator it = m_uniformLocationCache.find(uniformName);
	return it != m_uniformLocationCache.end() ? it->second : -1;
}
