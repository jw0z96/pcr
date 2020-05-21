#pragma once

#include <array>
#include <algorithm>

#include <GL/glew.h>

namespace GLUtils
{
	// hashes a string to a size_t, constexpr so it can be done at compile time
	constexpr size_t constStringHash(char const* input)
	{
		return *input ? static_cast<size_t>(*input) + 33 * constStringHash(input + 1) : 5381;
	}

	// Start a timer given it's hash, prefer the 'named' macro below to do compile time hashing
	void _startTimer(const size_t& hash);
#define startTimer(name) _startTimer(GLUtils::constStringHash(#name))

	// End a timer given it's hash, prefer the 'named' macro below to do compile time hashing
	void _endTimer(const size_t& hash);
#define endTimer(name) _endTimer(GLUtils::constStringHash(#name))

	// Helper for the scoped timer, starts the timer when it's constructed, ends it when it's destructed
	struct _scopedTimer
	{
		_scopedTimer(const size_t& h) : hash(h) { _startTimer(hash); }
		~_scopedTimer() { _endTimer(hash); }
		const size_t hash;
	};
	// TODO: what if someone calls this twice with the same name?
#define scopedTimer(name) _scopedTimer _timer_##name(GLUtils::constStringHash(#name))


	// Get a timer's elapsed value given it's hash, prefer the 'named' macro below to do compile time hashing
	float _getElapsed(const size_t& hash);
#define getElapsed(name) _getElapsed(GLUtils::constStringHash(#name))

	// awkwardly needed, since we'll need to ensure the destructors of the Timer objects in the static map are
	// called before the gl context is destroyed
	void clearTimers();

	class Timer
	{
	public:
		Timer();

		~Timer() { glDeleteQueries(s_bufferSize * 2, &m_queries.data()->start); }

		// Disable copy constructor and assignment operator, since we're managing OpenGL resources, and it's
		// not worth the hassle to share their ownership
		Timer(const Timer&) = delete;
		Timer& operator=(const Timer&) = delete;

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
