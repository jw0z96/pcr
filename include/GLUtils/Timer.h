#pragma once

#include <array>
#include <algorithm>

#include <GL/glew.h>


namespace GLUtils
{

// testing
unsigned constexpr const_hash(char const *input)
{
	return *input ? static_cast<unsigned int>(*input) + 33 * const_hash(input + 1) : 5381;
}

class Timer
{
public:
	Timer() : m_queries(), // default initialize elements
		m_elapsed(0)
	{
		glGenQueries(s_bufferSize * 2, &m_queries.data()->start);
	}

	~Timer() { glDeleteQueries(s_bufferSize * 2, &m_queries.data()->start); }

	void start() const
	{
		glQueryCounter(m_queries.front().start , GL_TIMESTAMP);
	}

	void end()
	{
		glQueryCounter(m_queries.front().end , GL_TIMESTAMP);

		// rotate the buffer left 1 element, maybe not necessary?
		std::rotate(m_queries.begin(), m_queries.begin() + 1, m_queries.end());

		GLint64 start, end;
		glGetQueryObjecti64v(m_queries.front().start, GL_QUERY_RESULT, &start);
		glGetQueryObjecti64v(m_queries.front().end, GL_QUERY_RESULT, &end);
		m_elapsed = end - start;
	}

	float elapsed() const
	{
		return m_elapsed / 1000000.0;
	}

private:
	struct TimerQuery
	{
		GLuint start, end;
	};

	static constexpr unsigned int s_bufferSize = 3;
	std::array<TimerQuery, s_bufferSize> m_queries;
	GLint64 m_elapsed;
};

} // namespace GLUtils
