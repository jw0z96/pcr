#include "GLUtils/Timer.h"

#include <iostream>
#include <unordered_map>

namespace
{
	// Since the intended usage is via the 'named' macros, which hash the strings at compile time,
	// we don't need to hash here... why are we using a map again?
	auto noHash = [](const size_t& i) { return i; };
	typedef std::unordered_map<size_t, GLUtils::Timer, decltype(noHash)> TimerMap;
	static TimerMap s_timers(10, noHash); // bucket size of 10?
} // namespace

void GLUtils::_startTimer(const size_t& id)
{
	// TimerMap::const_iterator it = s_timers.try_emplace(id).first;
	// it->second.start();
	s_timers[id].start(); // or this, since GLUtils::Timer is trivially constructed
}

void GLUtils::_endTimer(const size_t& id)
{
	TimerMap::iterator it = s_timers.find(id);
	if (it != s_timers.end())
	{
		it->second.end();
	}
	else // not found
	{
		std::cout << "GLUtils::endTimer: Timer wasn't found! hash ID: " << id << "\n";
	}
}

float GLUtils::_getElapsed(const size_t& id)
{
	TimerMap::const_iterator it = s_timers.find(id);

	/*
	if (it != s_timers.end())
	{
		return it->second.elapsed();
	}
	else // not found
	{
		std::cout<<"GLUtils::getElapsed: Timer wasn't found! hash ID: "<<id<<"\n";
	}
	return 0.0f;
	*/

	// it doesn't always make sense to print an error here
	return it != s_timers.end() ? it->second.elapsed() : 0.0f;
}

void GLUtils::clearTimers() { s_timers.clear(); }

GLUtils::Timer::Timer() :
	m_queries(), // default initialize elements
	m_elapsed(0.0f)
{
	glGenQueries(s_bufferSize * 2, &m_queries.data()->start);
}

void GLUtils::Timer::end()
{
	glQueryCounter(m_queries.front().end, GL_TIMESTAMP);

	// rotate the buffer left 1 element, maybe not necessary?
	std::rotate(m_queries.begin(), m_queries.begin() + 1, m_queries.end());

	GLint64 start, end;
	glGetQueryObjecti64v(m_queries.front().start, GL_QUERY_RESULT, &start);
	glGetQueryObjecti64v(m_queries.front().end, GL_QUERY_RESULT, &end);
	m_elapsed = (end - start) / 1000000.0;
}
