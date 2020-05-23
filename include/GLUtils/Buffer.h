#pragma once

#include <GL/glew.h>

// This just wraps a couple of OpenGL buffer manipulation methods,
// so that I don't have to touch the raw ID
// also ensures deletion when it goes out of scope

namespace GLUtils
{
	class Buffer
	{
	public:
		Buffer() : m_id(0) { glGenBuffers(1, &m_id); }

		~Buffer() { glDeleteBuffers(1, &m_id); }

		// Disable copy constructor and assignment operator, since we're managing OpenGL resources, and it's
		// not worth the hassle to share their ownership
		Buffer(const Buffer &) = delete;
		Buffer &operator=(const Buffer &) = delete;
		// ...and move constructor, move assignment
		Buffer(Buffer&&) = delete;
		Buffer& operator=(Buffer&&) = delete;

		inline void bindAs(const GLenum &type) const { glBindBuffer(type, m_id); }

		inline void bindAsIndexed(const GLenum &type, const GLuint &index) const
		{
			glBindBufferBase(type, index, m_id);
		}

		static inline void unbind(const GLenum &type) { glBindBuffer(type, 0); }

		// uh... expects a bound GL_TEXTURE_BUFFER texture object
		inline void attachToTextureBuffer(const GLenum &format) const
		{
			GLint tex = 0;
			glGetIntegerv(GL_TEXTURE_BINDING_BUFFER, &tex);
			if (!tex)
			{
				// TODO: Throw an error
				return;
			}

			glTexBuffer(GL_TEXTURE_BUFFER, format, m_id);
		}

	private:
		// Buffer ID
		GLuint m_id;
	};

} // namespace GLUtils
