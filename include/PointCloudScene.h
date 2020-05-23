#pragma once

#include <tinyply/tinyply.h>
#include "ply_utils.h"

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
	PointCloudScene() :
		m_idFBO(), m_idTexture(), m_depthTexture(), m_colourTexture(),
		m_visComputeShader({{GL_COMPUTE_SHADER, "shaders/visibility_comp.glsl"}}),
		m_pointsShader({{GL_VERTEX_SHADER, "shaders/points_vert.glsl"},
						{GL_FRAGMENT_SHADER, "shaders/points_frag.glsl"}}),
		m_outputShader({{GL_VERTEX_SHADER, "shaders/screenspace_vert.glsl"},
						{GL_FRAGMENT_SHADER, "shaders/output_frag.glsl"}}),
		m_pointCloudVAO(), m_modelMat(glm::translate(
								glm::rotate(glm::mat4(1.0), 3.14159f / 2.0f, glm::vec3(-1.0f, 0.0f, 0.0f)),
								glm::vec3(0.0f, 0.0f, -5.0f))),
		m_pointsBuffer(), m_colBuffer(), m_visBuffer(), m_elementBuffer(), m_counterBuffer(), m_camera(),
		m_computeDispatchCount(0), m_numPointsVisible(0), m_numPointsTotal(0), m_doProgressive(false)
	{
		// enable programmable point size in vertex shaders, no better place to put this?
		glEnable(GL_PROGRAM_POINT_SIZE);

		// TODO: initialize FBO & textures?
		setFramebufferParams(1024, 1024);

		// Set constant uniforms on the shaders
		m_pointsShader.use();
		glUniformMatrix4fv(
			m_pointsShader.getUniformLocation("projection"), 1, GL_FALSE,
			glm::value_ptr(m_camera.getProjection()));
		glUniformMatrix4fv(
			m_pointsShader.getUniformLocation("model"), 1, GL_FALSE, glm::value_ptr(m_modelMat));

		// TODO: map out texture units properly
		m_outputShader.use();
		// use texture unit 0 for the depth texture
		glActiveTexture(GL_TEXTURE0);
		m_idTexture.bindAs(GL_TEXTURE_2D);
		glUniform1i(m_outputShader.getUniformLocation("idTexture"), 0);
		// use texture unit 1 for the depth texture
		glActiveTexture(GL_TEXTURE1);
		m_depthTexture.bindAs(GL_TEXTURE_2D);
		glUniform1i(m_outputShader.getUniformLocation("depthTexture"), 1);
		// use texture unit 2 for the colour texture
		glActiveTexture(GL_TEXTURE2);
		m_colourTexture.bindAs(GL_TEXTURE_BUFFER);
		glUniform1i(m_outputShader.getUniformLocation("colTexture"), 2);

		// set up atomic counter
		GLuint counterValue = 0;
		m_counterBuffer.bindAs(GL_ATOMIC_COUNTER_BUFFER);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &counterValue, GL_DYNAMIC_DRAW);
		m_counterBuffer.bindAsIndexed(GL_ATOMIC_COUNTER_BUFFER, 0);
	}

	// ~PointCloudScene(); // let the compiler do it

	// Disable copy constructor and assignment operator, since we're managing external resources
	PointCloudScene(const PointCloudScene&) = delete;
	PointCloudScene& operator=(const PointCloudScene&) = delete;

	bool loadPointCloud(const char* filepath)
	{
		// read the vertex positions and colours from the ply file
		std::shared_ptr<tinyply::PlyData> plyPositions, plyColours;
		ply_utils::read_ply_file(filepath, plyPositions, plyColours);
		m_numPointsTotal = plyPositions ? plyPositions->count : 0;
		std::cout << "m_numPointsTotal: " << m_numPointsTotal << "\n";

		// TODO: check this is all correct, and that it rounds up to the nearest uint
		// we want 'm_numPointsTotal' bits to be allocated for the visibility buffer, but this has to be
		// allocated in bytes
		const size_t numVertsBytes = ceil(m_numPointsTotal / 8.0f); // 8 bits per byte
		std::cout << "num bytes: " << numVertsBytes << "\n";
		// determine workgroup count, we want 1 shader invocation per uint in the visibililty buffer
		int work_grp_cnt, work_grp_size;
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size);
		m_computeDispatchCount = ceil(numVertsBytes / 4);

		// we have to bind a VAO to hold the vertex attributes for the buffers, and the element buffer bindings
		m_pointCloudVAO.bind();

		// generate buffers for verts, set the VAO attributes
		m_pointsBuffer.bindAs(GL_ARRAY_BUFFER);
		glBufferData(
			GL_ARRAY_BUFFER, plyPositions->buffer.size_bytes(), plyPositions->buffer.get(), GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

		// generate buffers for colours
		m_colBuffer.bindAs(GL_TEXTURE_BUFFER);
		glBufferData(
			GL_TEXTURE_BUFFER, plyColours->buffer.size_bytes(), plyColours->buffer.get(), GL_STATIC_DRAW);
		// map it to a texture
		glActiveTexture(GL_TEXTURE2);
		m_colourTexture.bindAs(GL_TEXTURE_BUFFER);
		m_colBuffer.attachToTextureBuffer(GL_R8UI);

		// glActiveTexture(GL_TEXTURE1);
		// glBindTexture(GL_TEXTURE_BUFFER, m_colourTexture);
		// glTexBuffer(GL_TEXTURE_BUFFER, GL_R8UI, m_colBuffer);
		// glBindBuffer(GL_TEXTURE_BUFFER, 0);
		// optionally, enable it as a vertex attribute
		// glBindBuffer(GL_ARRAY_BUFFER, m_colBuffer);
		// glEnableVertexAttribArray(1);
		// glVertexAttribIPointer(1, 3, GL_UNSIGNED_BYTE, 3 * sizeof(unsigned char), (void*)0); // sized type

		// generate an SSBO for the visibility
		m_visBuffer.bindAs(GL_SHADER_STORAGE_BUFFER);
		glBufferData(GL_SHADER_STORAGE_BUFFER, numVertsBytes, nullptr, GL_DYNAMIC_COPY);
		m_visBuffer.bindAsIndexed(GL_SHADER_STORAGE_BUFFER, 0);

		// Generate an element buffer for the point indices to redraw
		m_elementBuffer.bindAs(GL_ELEMENT_ARRAY_BUFFER);
		// give it enough elements for a full draw
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_numPointsTotal * sizeof(GLuint), nullptr, GL_DYNAMIC_COPY);
		// Also bind it as a SSBO so we can use it in the compute shader
		m_elementBuffer.bindAs(GL_SHADER_STORAGE_BUFFER);
		m_elementBuffer.bindAsIndexed(GL_SHADER_STORAGE_BUFFER, 1);

		std::cout << "gl error: " << glGetError() << "\n"; // TODO: A proper macro for glErrors

		return true;
	}

	void processEvent(const SDL_Event& event)
	{
		if (event.type == SDL_WINDOWEVENT &&
			(event.window.event == SDL_WINDOWEVENT_RESIZED ||
			 event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED))
		{
			setFramebufferParams(event.window.data1, event.window.data2); // implicit cast to uint
			m_camera.setAspect(event.window.data1, event.window.data2);
			m_pointsShader.use();
			glUniformMatrix4fv(
				m_pointsShader.getUniformLocation("projection"), 1, GL_FALSE,
				glm::value_ptr(m_camera.getProjection()));
		}
		m_camera.processInput(event);
	}

	void setFramebufferParams(const unsigned int& width, const unsigned int& height)
	{
		if (!initIndexFramebuffer(width, height))
		{
			std::cout << "error resizing index framebuffer\n";
		}

		GLUtils::Framebuffer::bindDefault();
		glViewport(0, 0, width, height);
	}

	void drawScene()
	{
		GLUtils::scopedTimer(newFrameTimer);

		// ID Pass
		{
			GLUtils::scopedTimer(idPassTimer);

			// ID Compute EBO pass
			if (m_doProgressive)
			{
				GLUtils::scopedTimer(indexComputeTimer);
				// use a compute shader to count the elements in the visibility buffer
				m_visComputeShader.use();
				{
					GLUtils::scopedTimer(indexComputeDispatchTimer);
					glDispatchCompute(m_computeDispatchCount, 1, 1);
				}
				// get the atomic counter value
				{
					GLUtils::scopedTimer(indexCounterReadTimer);
					glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &m_numPointsVisible);
				}
				// reset the counter value
				{
					GLUtils::scopedTimer(indexCounterResetTimer);
					static const GLuint zero = 0;
					glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &zero, GL_DYNAMIC_DRAW);
				}
			}

			// ID Draw Pass
			{
				GLUtils::scopedTimer(pointsDrawTimer);

				m_idFBO.bind();
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				glEnable(GL_DEPTH_TEST);
				// update point shader view uniform with new view matrix
				m_pointsShader.use();
				glUniformMatrix4fv(
					m_pointsShader.getUniformLocation("view"), 1, GL_FALSE,
					glm::value_ptr(m_camera.getView()));

				// dispatch point draw
				if (m_doProgressive)
				{
					glDrawElements(GL_POINTS, m_numPointsVisible, GL_UNSIGNED_INT, nullptr);
				}
				else
				{
					glDrawArrays(GL_POINTS, 0, m_numPointsTotal);
				}
			}
		}

		// Output Pass
		{
			GLUtils::scopedTimer(outputPassTimer);
			GLUtils::Framebuffer::bindDefault();
			glClear(GL_COLOR_BUFFER_BIT);
			glDisable(GL_DEPTH_TEST);
			m_outputShader.use();
			// The vertex shader will create a screen space quad, so no need to bind a different VAO & VBO
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
	}

	void drawGUI()
	{
		// stats window
		ImGui::Begin("Controls");
		ImGui::Checkbox("Progressive Render", &m_doProgressive);

		// if (m_doProgressive)
		// {
		// 	ImGui::Text("Fill Rate (per frame):");
		// 	ImGui::SliderInt("", &fillRate, 0, maxFillRate);
		// }

		ImGui::Separator();

		ImGui::Text(
			"Drawing %u / %u points (%.2f%%)", m_doProgressive ? m_numPointsVisible : m_numPointsTotal,
			m_numPointsTotal, m_doProgressive ? (m_numPointsVisible * 100.0f / m_numPointsTotal) : 100.0f);

		ImGui::Separator();

		const float frameTime = GLUtils::getElapsed(newFrameTimer);
		ImGui::Text("Frame time: %.1f ms (%.1f fps)", frameTime, 1000.0f / frameTime);
		ImGui::Text("	ID Pass time: %.1f ms", GLUtils::getElapsed(idPassTimer));
		if (m_doProgressive)
		{
			ImGui::Text("		Index Compute time: %.1f ms", GLUtils::getElapsed(indexComputeTimer));
			ImGui::Text(
				"			Index Compute Dispatch time: %.1f ms",
				GLUtils::getElapsed(indexComputeDispatchTimer));
			ImGui::Text(
				"			Index Counter Read time: %.1f ms", GLUtils::getElapsed(indexCounterReadTimer));
			ImGui::Text(
				"			Index Counter Reset time: %.1f ms", GLUtils::getElapsed(indexCounterResetTimer));
		}
		ImGui::Text("		Points Draw time: %.1f ms", GLUtils::getElapsed(pointsDrawTimer));
		ImGui::Text("	Output Pass time: %.1f ms", GLUtils::getElapsed(outputPassTimer));
		ImGui::End();
	}

private:
	bool initIndexFramebuffer(const unsigned int& width, const unsigned int& height)
	{
		m_idFBO.bind();
		glViewport(0, 0, width, height); // again?

		// create integer id texture
		glActiveTexture(GL_TEXTURE0);
		m_idTexture.bindAs(GL_TEXTURE_2D);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, width, height, 0, GL_RED_INTEGER, GL_INT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		// attach it to FBO
		m_idTexture.attachToFrameBuffer(GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);

		// create depth attachment, because we can't enable depth testing otherwise
		glActiveTexture(GL_TEXTURE1);
		m_depthTexture.bindAs(GL_TEXTURE_2D);
		// glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
		// glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT,
		// GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		// attach it to FBO
		m_depthTexture.attachToFrameBuffer(GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D);

		// std::cout<<"gl error:"<<glGetError()<<"\n";

		// tell OpenGL which attachments we'll use (of this framebuffer) for rendering
		const std::array<GLenum, 1> attachments = {
			GL_COLOR_ATTACHMENT0}; // we don't need to list the depth attachment
		glDrawBuffers(1, attachments.data());

		bool success = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
		if (!success)
		{
			std::cout << "Error: Framebuffer is not complete!\n";
		}

		// unbind framebuffer before we return
		GLUtils::Framebuffer::bindDefault();
		return success;
	}

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
