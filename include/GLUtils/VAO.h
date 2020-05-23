#pragma once

#include <GL/glew.h>

// This just wraps a couple of OpenGL VAO manipulation methods,
// so that I don't have to touch the raw ID
// also ensures deletion when it goes out of scope

namespace GLUtils
{
	class VAO
	{
	public:
		VAO() : m_id(0) { glGenVertexArrays(1, &m_id); }

		~VAO() { glDeleteVertexArrays(1, &m_id); }

		// Disable copy constructor and assignment operator, since we're managing OpenGL resources, and it's
		// not worth the hassle to share their ownership
		VAO(const VAO &) = delete;
		VAO &operator=(const VAO &) = delete;

		inline void bind() const { glBindVertexArray(m_id); }

		static inline void unbind() { glBindVertexArray(0); }

	private:
		// VAO ID
		GLuint m_id;
	};

} // namespace GLUtils
