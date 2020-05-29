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

	explicit ShaderProgram(const std::list<ShaderComponent>& components);

	~ShaderProgram()
	{
		glDeleteProgram(m_shaderProgramID);
	}

	// Disable copy constructor and assignment operator, since we're managing OpenGL resources, and it's
	// not worth the hassle to share their ownership
	ShaderProgram(const ShaderProgram&) = delete;
	ShaderProgram& operator=(const ShaderProgram&) = delete;
	// ...and move constructor, move assignment
	ShaderProgram(ShaderProgram&&) = delete;
	ShaderProgram& operator=(ShaderProgram&&) = delete;

	void use() const;

	// Make sure you use this shader program before using the returned value in glUniform...
	GLint getUniformLocation(const char* uniformName) const;

private:
	// Cache uniform locations
	std::unordered_map<std::string, GLint> m_uniformLocationCache;

	// Shader program ID
	const GLuint m_shaderProgramID;

	// Track whether the shader is valid
	bool m_isValid;
};

} // namespace GLUtils
