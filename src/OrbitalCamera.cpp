#include "OrbitalCamera.h"

#include <glm/gtc/matrix_transform.hpp>

// This is annoying, but glm constants aren't constexpr
namespace
{
constexpr double TWO_PI = 2.0f * M_PI;
constexpr double HALF_PI = 0.5f * M_PI;
static const glm::vec3 s_worldUp(0.0, 1.0, 0.0); // glm::vec3() is non-constexpr?
} // namespace

OrbitalCamera::OrbitalCamera()
	: m_theta(0.0f)
	, m_phi(0.0f)
	, m_target(0.0f)
	, m_distance(20.0f)
	, m_viewMat(std::nullopt)
	, m_fov(45.0f)
	, m_aspect(1.0f)
	, m_nearClip(1.0f)
	, m_farClip(100.0f)
	, m_projectionMat(std::nullopt)
{}

void OrbitalCamera::processInput(const SDL_Event& event)
{
	// mouse sensitivity multiplier for the camera rotation control
	constexpr float s_mouseSensitivity = 0.001f;
	// scroll sensitivity multiplier for the camera distance control
	constexpr float s_scrollSensitivity = 1.0f;

	switch(event.type)
	{
	case SDL_MOUSEMOTION: {
		if(event.motion.state &
			SDL_BUTTON_RMASK) // right button so we don't conflict with ImGui - lazy!
		{
			m_theta -= event.motion.xrel * s_mouseSensitivity;
			m_theta = glm::mod(m_theta, TWO_PI);

			m_phi += event.motion.yrel * s_mouseSensitivity;
			m_phi = glm::clamp(m_phi, -HALF_PI, HALF_PI);

			m_viewMat.reset();
		}
	}
	break;
	case SDL_MOUSEWHEEL: {
		m_distance -= event.wheel.y * s_scrollSensitivity;
		m_distance = glm::clamp(m_distance, m_nearClip, m_farClip); // lazy
		m_viewMat.reset();
	}
	default:
		break;
	}
}

const glm::mat4 OrbitalCamera::calculateView() const
{
	glm::vec3 viewPoint(
		glm::sin(m_theta) * glm::cos(m_phi), glm::sin(m_phi), glm::cos(m_theta) * glm::cos(m_phi));
	return glm::lookAt(viewPoint * m_distance, m_target, s_worldUp);
}

const glm::mat4 OrbitalCamera::calculateProjection() const
{
	return glm::perspective(glm::radians(m_fov), m_aspect, m_nearClip, m_farClip);
}