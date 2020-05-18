#pragma once

#include <array>
#include <algorithm>

#include <GL/glew.h>

namespace GLUtils
{
	// testing
	size_t constexpr constStringHash(char const* input)
	{
		return *input ? static_cast<size_t>(*input) + 33 * constStringHash(input + 1) : 5381;
	}

	// Start a timer given it's hash, prefer the 'named' macro below to do compile time hashing
	void _startTimer(const size_t& id);
#define startTimer(a) _startTimer(GLUtils::constStringHash(#a))

	// End a timer given it's hash, prefer the 'named' macro below to do compile time hashing
	void _endTimer(const size_t& id);
#define endTimer(a) _endTimer(GLUtils::constStringHash(#a))

	// A timer which starts upon construction, and ends when it goes out of scope...
	struct _scopedTimer
	{
		~_scopedTimer() { _endTimer(id); }
		const size_t id;
	};
	// TODO: what if someone calls this twice with the same name?
#define scopedTimer(a) \
	_scopedTimer timer_##a = {GLUtils::constStringHash(#a)}; \
	GLUtils::_startTimer(timer_##a.id)

	// Get a timer's elapsed value given it's hash, prefer the 'named' macro below to do compile time hashing
	float _getElapsed(const size_t& id);
#define getElapsed(a) _getElapsed(GLUtils::constStringHash(#a))

	// awkwardly needed, since we'll need to ensure the destructors of the Timer objects in the static map are
	// called before the gl context is destroyed
	void clearTimers();

	class Timer
	{
	public:
		Timer();

		~Timer() { glDeleteQueries(s_bufferSize * 2, &m_queries.data()->start); }

		void start() const { glQueryCounter(m_queries.front().start, GL_TIMESTAMP); }

		void end();

		float elapsed() const { return m_elapsed; }

	private:
		struct TimerQuery
		{
			GLuint start, end;
		};

		static constexpr unsigned int s_bufferSize = 3;
		std::array<TimerQuery, s_bufferSize> m_queries;
		float m_elapsed;
	};

} // namespace GLUtils
