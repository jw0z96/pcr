#ifndef SHADERPROGRAM_H
#define SHADERPROGRAM_H

#include <GL/glew.h>

#include <fstream>
#include <sstream>
#include <string>
#include <list>

class ShaderProgram
{
public:

	ShaderProgram() : m_shaderProgramID(glCreateProgram())
	{}

	// ~ShaderProgram() {}; // let the compiler do this :)
	inline ~ShaderProgram() { glDeleteProgram(m_shaderProgramID); }

	inline void addShaderSource(GLenum shaderType, const std::string &shaderPath)
	{
		const ShaderComponent component = {shaderType, shaderPath};
		m_shaderComponents.push_back(component); // emplace_back doesn't work with struct {} ctors
	}

	inline bool compile()
	{
		// detach all existing shaders
		{
			GLuint existingShaders[16]; // max num of attachments reasonable...
			GLsizei count;
			glGetAttachedShaders(m_shaderProgramID, 16, &count, existingShaders);
			if (count > 16)
			{
				std::cout<<"Error: cannot detach all shaders!\n";
			}

			for (GLsizei i = 0; i < count; ++i)
			{
				glDetachShader(m_shaderProgramID, existingShaders[i]);
			}
		}

		// load all shaders from disk and attach
		for (const ShaderComponent & component : m_shaderComponents)
		{
			// ensure ifstream objects can throw exceptions:
			std::ifstream shaderFile;
			shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
			std::stringstream shaderStream;
			try
			{
				shaderFile.open(component.source);
				shaderStream << shaderFile.rdbuf();
				shaderFile.close();
			}
			catch (std::ifstream::failure e)
			{
				std::cout << "Error: Whilst reading shader file " << component.source << "\n";
				return false; // unsuccessful
			}

			const std::string streamStr = shaderStream.str(); // ugh
			const char * sourceStr = streamStr.c_str(); // ugh

			const GLuint shader = glCreateShader(component.type);
			glShaderSource(shader, 1, &sourceStr, NULL);
			glCompileShader(shader);

			int shaderSuccess;
			glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderSuccess);
			if (!shaderSuccess)
			{
				char log[512];
				glGetShaderInfoLog(shader, 512, NULL, log);
				std::cout << "Error: compiling shader component of type " << component.type << ": " << log << "\n";
				glDeleteShader(shader);
				// glDeleteProgram(program);
				return false; // unsuccessful
			}

			// attach the shader to the shader program, and then delete it,
			// since it will be kept alive by the program
			glAttachShader(m_shaderProgramID, shader);
			glDeleteShader(shader);
		}

		// link the shaders to the program and
		glLinkProgram(m_shaderProgramID);

		int programSuccess;
		glGetProgramiv(m_shaderProgramID, GL_LINK_STATUS, &programSuccess);
		if(!programSuccess)
		{
			char log[512];
			glGetProgramInfoLog(m_shaderProgramID, 512, NULL, log);
			std::cout << "Error linking shader program: " << log << "\n";
			return false; // unsuccessful
		}

		return true;
	}

	inline void use() const
	{
		// if (m_shaderProgramID)
		// {
			glUseProgram(m_shaderProgramID);
		// }
		// else
		// {
			// std::cout << "Error: invalid shader cannot be used!\n";
		// }
	}

	inline void clearComponents()
	{
		m_shaderComponents.clear();
	}

	inline GLint getUniformLocation(const char * uniformName) const
	{
		// if (!m_shaderProgramID)
		// {
		// 	std::cout << "Error: getUniformLocation called on invalid shader!\n";
		// 	return 0;
		// }
		return glGetUniformLocation(m_shaderProgramID, uniformName);
	}

private:

	struct ShaderComponent
	{
		const GLuint type;
		const std::string source;
	};

	// map of shader types, and a supplied source code
	std::list<ShaderComponent> m_shaderComponents;

	// Shader program ID
	const GLuint m_shaderProgramID;
};

#endif // SHADERPROGRAM_H