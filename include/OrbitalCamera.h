#pragma once

#include <SDL2/SDL.h>

#include <glm/glm.hpp>

#include <optional>

// This just combines the code for calculating the view and projection matrices, but really they're
// independent of each other

class OrbitalCamera
{
public:
	OrbitalCamera();

	void processInput(const SDL_Event& event);

	// View Matrix methods
	inline void setCenter(const glm::vec3& target)
	{
		m_target = target;
	}

	// distance from target, a 'dolly' of sorts
	inline void setDistance(float distance)
	{
		m_distance = distance;
	}

	glm::mat4 getView() const
	{
		// not sure why this doesn't work, whilst returning a reference:
		// return m_viewMat.value_or(m_viewMat.emplace(calculateView()));
		return m_viewMat.has_value() ? m_viewMat.value() : m_viewMat.emplace(calculateView());
	}

	// Projection Matrix methods
	// FOV angle in degrees
	inline void setFOV(float fov)
	{
		m_fov = fov;
		m_projectionMat.reset();
	}

	// Make sure to call this when the aspect ratio of the window changes
	inline void setAspect(float aspect)
	{
		m_aspect = aspect;
		m_projectionMat.reset();
	}
	inline void setAspect(float width, float height)
	{
		m_aspect = width / height;
		m_projectionMat.reset();
	}

	inline void setNearFarClip(float near, float far)
	{
		m_nearClip = near;
		m_farClip = far;
		m_projectionMat.reset();
	}

	inline glm::mat4 getProjection() const
	{
		return m_projectionMat.has_value() ? m_projectionMat.value()
										   : m_projectionMat.emplace(calculateProjection());
	}

private:
	// Recalculate the view matrix
	const glm::mat4 calculateView() const;

	// Recalculate the projection matrix
	const glm::mat4 calculateProjection() const;

	// View Matrix variables
	// theta being the azimuthal angle, phi being altitude/elevation angle, in radians
	// see: https://i.stack.imgur.com/xA6Im.png (Y/Z swapped)
	// used for calculating the view point on a unit sphere
	double m_theta, m_phi;

	// center of the orbital camera
	glm::vec3 m_target;

	// distance from m_target
	float m_distance;

	// The actual view matrix... we make this a std::optional so that we can use std::optional's nullopt as a
	// 'dirty' state flag for when it needs to be recalculated
	mutable std::optional<const glm::mat4> m_viewMat;

	// Projection Matrix variables
	// projection FOV angle in degrees
	float m_fov;

	// projection aspect ratio
	float m_aspect;

	// projection near and far clipping distances
	float m_nearClip, m_farClip;

	// The actual projection matrix... mutable std::optional as above
	mutable std::optional<const glm::mat4> m_projectionMat;
};