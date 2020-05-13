#pragma once

#include <GL/glew.h>
#include <list>
#include <string>
#include <unordered_map>

// This just wraps the OpenGL Shader & ShaderProgram creation methods,
// so that I don't have to touch the raw ID
// also ensures deletion when it goes out of scope

namespace GLUtils
{
class ShaderProgram
{
public:
	struct ShaderComponent
	{
		const GLenum type;
		const char* source;
	};

	ShaderProgram(const std::list<ShaderComponent>& components);

	inline ~ShaderProgram() { glDeleteProgram(m_shaderProgramID); }

	// TODO: Check if shader is valid?
	inline void use() const { glUseProgram(m_shaderProgramID); }

	// Make sure you use this shader program before using the returned value in glUniform...
	GLint getUniformLocation(const char* uniformName);

private:
	// Cache uniform locations
	std::unordered_map<std::string, GLint> m_uniformLocationCache;

	// Shader program ID
	const GLuint m_shaderProgramID;
};

} // namespace GLUtils
