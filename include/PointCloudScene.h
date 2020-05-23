#pragma once

#include "GLUtils/Buffer.h"
#include "GLUtils/Texture.h"
#include "GLUtils/VAO.h"
#include "GLUtils/Framebuffer.h"
#include "GLUtils/ShaderProgram.h"
#include "GLUtils/Timer.h"

#include "OrbitalCamera.h"

class PointCloudScene
{
public:
	PointCloudScene();

	// ~PointCloudScene(); // let the compiler do it

	// Disable copy constructor and assignment operator, since we're managing external resources
	PointCloudScene(const PointCloudScene&) = delete;
	PointCloudScene& operator=(const PointCloudScene&) = delete;

	bool loadPointCloud(const char* filepath);

	void processEvent(const SDL_Event& event);

	void setFramebufferParams(const unsigned int& width, const unsigned int& height);

	void drawScene();

	void drawGUI();

private:
	bool initIndexFramebuffer(const unsigned int& width, const unsigned int& height);

	const GLUtils::Framebuffer m_idFBO;

	const GLUtils::Texture m_idTexture, m_depthTexture, m_colourTexture;

	const GLUtils::ShaderProgram m_visComputeShader, m_pointsShader, m_outputShader;

	const GLUtils::VAO m_pointCloudVAO;

	const glm::mat4 m_modelMat; // Model matrix to roughly center and orient the point cloud...

	const GLUtils::Buffer m_pointsBuffer, m_colBuffer, m_visBuffer, m_elementBuffer, m_counterBuffer;

	OrbitalCamera m_camera;

	GLuint m_computeDispatchCount;
	GLuint m_numPointsVisible;
	GLuint m_numPointsTotal;

	bool m_doProgressive;
};
