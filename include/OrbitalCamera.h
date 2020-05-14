#pragma once

#include <SDL2/SDL.h>

#include <glm/glm.hpp>

// #include <cmath>

class OrbitalCamera
{
public:
	OrbitalCamera() :
		m_theta(0.0f), m_phi(0.0f), m_target(0.0f), m_distance(20.0f), m_viewMat(1.0f), m_fov(45.0f), m_aspect(1.0f),
		m_nearClip(1.0f), m_farClip(100.0f), m_projectionMat(1.0f)
	{
		updateView();
		updateProjection();
	}

	// ~OrbitalCamera();

	// View Matrix methods

	void processInput(const SDL_Event& event)
	{
		// this is ugly
		switch (event.type)
		{
			case SDL_MOUSEMOTION:
			{
				if (event.motion.state & SDL_BUTTON_LMASK)
				{
					m_theta -= event.motion.xrel * 0.001f;
					m_theta = glm::mod(m_theta, 2.0f * glm::pi<double>());

					m_phi += event.motion.yrel * 0.001f;
					m_phi = glm::clamp(m_phi, -0.5 * glm::pi<double>(), 0.5 * glm::pi<double>());

					// std::cout<<"m_theta: "<<m_theta<<", m_phi: "<<m_phi<<"\n";
					updateView();
				}
			}
			break;
			case SDL_MOUSEWHEEL:
			{
				m_distance = glm::clamp(m_distance - event.wheel.y, m_nearClip, m_farClip); // lazy
				updateView();
			}
			default: break;
		}
	}

	void setCenter(const glm::vec3& target) { m_target = target; }

	// distance from target
	void setDistance(float distance) { m_distance = distance; }

	glm::mat4 getView() const { return m_viewMat; }

	// Projection Matrix methods

	// degrees
	void setFOV(float fov) { m_fov = fov; }

	// Make sure to call this when the aspect ratio of the window changes
	void setAspect(float aspect) { m_aspect = aspect; }
	// void setAspect(float width, float height) { m_aspect = width / height; }

	void setNearFarClip(float near, float far)
	{
		m_nearClip = near;
		m_farClip = far;
	}

	glm::mat4 getProjection() const { return m_projectionMat; }

private:
	void updateView()
	{
		glm::vec3 point;
		point.x = glm::sin(m_theta) * glm::cos(m_phi);
		point.y = glm::sin(m_phi);
		point.z = glm::cos(m_theta) * glm::cos(m_phi);

		point *= m_distance;
		m_viewMat = glm::lookAt(point, m_target, glm::vec3(0.0, 1.0, 0.0));
	}

	void updateProjection()
	{
		m_projectionMat = glm::perspective(glm::radians(m_fov), m_aspect, m_nearClip, m_farClip);
	}

	// static const glm::vec3 s_worldUp;

	// theta being the azimuthal angle, phi being altitude/elevation angle, in radians
	// see: https://i.stack.imgur.com/xA6Im.png (Y/Z swapped)
	double m_theta, m_phi;

	glm::vec3 m_target;
	float m_distance;
	glm::mat4 m_viewMat;

	float m_fov, m_aspect, m_nearClip, m_farClip;
	glm::mat4 m_projectionMat;
};