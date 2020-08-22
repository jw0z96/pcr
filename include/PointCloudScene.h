#pragma once

#include "GLUtils/Buffer.h"
#include "GLUtils/Framebuffer.h"
#include "GLUtils/ShaderProgram.h"
#include "GLUtils/Texture.h"
#include "GLUtils/Timer.h"
#include "GLUtils/VAO.h"

#include "OrbitalCamera.h"

class PointCloudScene
{
public:
	PointCloudScene();

	// ~PointCloudScene(); // let the compiler do it

	bool loadPointCloud(const char* filepath);

	void processEvent(const SDL_Event& event);

	void setFramebufferParams(const unsigned int& width, const unsigned int& height);

	void drawScene();

	void drawGUI();

private:
	struct DrawElementsIndirectCommand
	{
		GLuint count;
		GLuint primCount;
		GLuint firstIndex;
		GLint baseVertex;
		GLuint reservedMustBeZero;
	};

	struct DispatchIndirectCommand
	{
		GLuint num_groups_x;
		GLuint num_groups_y;
		GLuint num_groups_z;
	};

	bool initIndexFramebuffer(const unsigned int& width, const unsigned int& height);

	const GLUtils::Framebuffer m_idFBO;

	const GLUtils::Texture m_idTexture, m_depthTexture, m_colourTexture;

	const GLUtils::ShaderProgram m_visComputeShader, m_elementComputeShader, m_pointsShader,
		m_outputShader;

	const GLUtils::VAO m_pointCloudVAO;

	const glm::mat4 m_modelMat; // Model matrix to roughly center and orient the point cloud...

	const GLUtils::Buffer m_pointsBuffer, m_colBuffer, m_visBuffer, m_elementBuffer,
		m_shuffledBuffer, m_indirectElementsBuffer, m_indirectComputeBuffer;

	OrbitalCamera m_camera;

	GLuint m_computeDispatchCount;
	GLuint m_computeGroupCount;
	GLuint m_numPointsVisible;
	GLuint m_numPointsTotal;

	bool m_doProgressive, m_doShuffle;
	unsigned int m_fillStartIndex;
	float m_fillRate;
	float m_pointSize;
};
