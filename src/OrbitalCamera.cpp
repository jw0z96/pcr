#include "OrbitalCamera.h"

#include <glm/gtc/matrix_transform.hpp>

// This is annoying, but glm constants aren't constexpr
namespace
{
	constexpr double TWO_PI = 2.0f * M_PI;
	constexpr double HALF_PI = 0.5f * M_PI;
} // namespace

const glm::vec3 OrbitalCamera::s_worldUp = glm::vec3(0.0, 1.0, 0.0);
const float OrbitalCamera::s_mouseSensitivity = 0.001f;
const float OrbitalCamera::s_scrollSensitivity = 1.0f;

OrbitalCamera::OrbitalCamera() :
	m_theta(0.0f), m_phi(0.0f), m_target(0.0f), m_distance(20.0f), m_viewMat(1.0f), m_fov(45.0f),
	m_aspect(1.0f), m_nearClip(1.0f), m_farClip(100.0f), m_projectionMat(1.0f)
{
	updateView();
	updateProjection();
}

void OrbitalCamera::processInput(const SDL_Event& event)
{
	switch (event.type)
	{
		case SDL_MOUSEMOTION:
		{
			if (event.motion.state & SDL_BUTTON_LMASK)
			{
				m_theta -= event.motion.xrel * s_mouseSensitivity;
				m_theta = glm::mod(m_theta, TWO_PI);

				m_phi += event.motion.yrel * s_mouseSensitivity;
				m_phi = glm::clamp(m_phi, -HALF_PI, HALF_PI);

				// std::cout<<"m_theta: "<<m_theta<<", m_phi: "<<m_phi<<"\n";
				updateView();
			}
		}
		break;
		case SDL_MOUSEWHEEL:
		{
			m_distance -= event.wheel.y * s_scrollSensitivity;
			m_distance = glm::clamp(m_distance, m_nearClip, m_farClip); // lazy
			updateView();
		}
		default: break;
	}
}

void OrbitalCamera::updateView()
{
	glm::vec3 point(
		glm::sin(m_theta) * glm::cos(m_phi), glm::sin(m_phi), glm::cos(m_theta) * glm::cos(m_phi));

	point *= m_distance;
	m_viewMat = glm::lookAt(point, m_target, s_worldUp);
}

void OrbitalCamera::updateProjection()
{
	m_projectionMat = glm::perspective(glm::radians(m_fov), m_aspect, m_nearClip, m_farClip);
}