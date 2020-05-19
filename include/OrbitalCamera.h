#pragma once

#include <SDL2/SDL.h>

#include <glm/glm.hpp>

// This just combines the code for calculating the view and projection matrices, but really they're
// independent of each other

class OrbitalCamera
{
public:
	OrbitalCamera();

	void processInput(const SDL_Event& event);

	// View Matrix methods
	inline void setCenter(const glm::vec3& target) { m_target = target; }

	// distance from target, a 'dolly' of sorts
	inline void setDistance(float distance) { m_distance = distance; }

	inline glm::mat4 getView() const { return m_viewMat; }

	// Projection Matrix methods
	// FOV angle in degrees
	inline void setFOV(float fov) { m_fov = fov; }

	// Make sure to call this when the aspect ratio of the window changes
	inline void setAspect(float aspect) { m_aspect = aspect; }
	inline void setAspect(float width, float height) { m_aspect = width / height; }

	inline void setNearFarClip(float near, float far)
	{
		m_nearClip = near;
		m_farClip = far;
	}

	inline glm::mat4 getProjection() const { return m_projectionMat; }

private:
	// Recalculate the view matrix
	void updateView();

	// Recalculate the projection matrix
	void updateProjection();

	// View Matrix variables
	// world up-vector used when calculating the view matrix
	static const glm::vec3 s_worldUp;

	// mouse sensitivity multiplier for the camera rotation control
	static const float s_mouseSensitivity;

	// scroll sensitivity multiplier for the camera distance control
	static const float s_scrollSensitivity;

	// theta being the azimuthal angle, phi being altitude/elevation angle, in radians
	// see: https://i.stack.imgur.com/xA6Im.png (Y/Z swapped)
	// used for calculating the view point on a unit sphere
	double m_theta, m_phi;

	// center of the orbital camera
	glm::vec3 m_target;

	// distance from m_target
	float m_distance;

	// the actual view matrix...
	glm::mat4 m_viewMat;

	// Projection Matrix variables
	// projection FOV angle in degrees
	float m_fov;

	// projection aspect ratio
	float m_aspect;

	// projection near and far clipping distances
	float m_nearClip, m_farClip;

	// the actual projection matrix...
	glm::mat4 m_projectionMat;
};