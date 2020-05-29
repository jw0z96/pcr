#pragma once

#include <GL/glew.h>

// This just wraps a couple of OpenGL Framebuffer manipulation methods,
// so that I don't have to touch the raw ID
// also ensures deletion when it goes out of scope

namespace GLUtils
{
class Framebuffer
{
public:
	Framebuffer()
		: m_id(0)
	{
		glGenFramebuffers(1, &m_id);
	}

	~Framebuffer()
	{
		glDeleteFramebuffers(1, &m_id);
	}

	// Disable copy constructor and assignment operator, since we're managiang OpenGL resources, and it's
	// not worth the hassle to share their ownership
	Framebuffer(const Framebuffer&) = delete;
	Framebuffer& operator=(const Framebuffer&) = delete;
	// ...and move constructor, move assignment
	Framebuffer(Framebuffer&&) = delete;
	Framebuffer& operator=(Framebuffer&&) = delete;

	inline void bind() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_id);
	}

	// use this to 'unbind', since ID 0 represents the default framebuffer
	static inline void bindDefault()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

private:
	// Framebuffer ID
	GLuint m_id;
};

} // namespace GLUtils
