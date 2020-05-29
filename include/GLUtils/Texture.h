#pragma once

#include <GL/glew.h>

// This just wraps a couple of OpenGL Texture manipulation methods,
// so that I don't have to touch the raw ID
// also ensures deletion when it goes out of scope

namespace GLUtils
{
class Texture
{
public:
	Texture()
		: m_id(0)
	{
		glGenTextures(1, &m_id);
	}

	~Texture()
	{
		glDeleteTextures(1, &m_id);
	}

	// Disable copy constructor and assignment operator, since we're managing OpenGL resources, and it's
	// not worth the hassle to share their ownership
	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;
	// ...and move constructor, move assignment
	Texture(Texture&&) = delete;
	Texture& operator=(Texture&&) = delete;

	inline void bindAs(const GLenum& type) const
	{
		glBindTexture(type, m_id);
	}

	static inline void unbind(const GLenum& type)
	{
		glBindTexture(type, 0);
	}

	// uh... expects a bound GL_TEXTURE_BUFFER texture object
	inline void attachToFrameBuffer(const GLenum& attachment, const GLenum& type) const
	{
		GLint tex = 0;
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &tex); // also GL_READ_FRAMEBUFFER_BINDING
		if(tex == 0)
		{
			// TODO: Throw an error, we can't attach to the default framebuffer
			return;
		}

		glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, type, m_id, 0);
	}

private:
	// Texture ID
	GLuint m_id;
};

} // namespace GLUtils
