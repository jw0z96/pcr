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

	ShaderProgram() : m_shaderProgramID(0) // this shouldn't be necessary, but i've been burned in the past
	{}

	// ~ShaderProgram() {}; // let the compiler do this :)

	inline void loadShaderSource(GLenum shaderType, const std::string &shaderPath)
	{
		// ensure ifstream objects can throw exceptions:
		std::ifstream shaderFile;
		shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		std::stringstream shaderStream;
		try
		{
			shaderFile.open(shaderPath);
			shaderStream << shaderFile.rdbuf();
			shaderFile.close();
		}
		catch(std::ifstream::failure e)
		{
			std::cout << "Error: Whilst reading shader file " << shaderPath << "\n";
			return;
		}

		const std::string & shaderSource = shaderStream.str();
		if (shaderSource.empty())
		{
			std::cout << "Error: Shader file " << shaderPath << " is empty!\n";
		}
		else
		{
			const ShaderComponent component = {shaderType, shaderSource};
			m_shaderComponents.push_back(component); // emplace_back doesn't work with struct {} ctors
		}
	}

	inline bool compile()
	{
		// create a new shader program
		const GLuint program = glCreateProgram();

		for (const ShaderComponent & component : m_shaderComponents)
		{
			const GLuint shader = glCreateShader(component.type);
			const char * sourceStr = component.source.c_str(); // ugh
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
				return false; // unsuccessful
			}

			// attach the shader to the shader program, and then delete it,
			// since it will be kept alive by the program
			glAttachShader(program, shader);
			glDeleteShader(shader);
		}

		// link the shaders to the program and
		glLinkProgram(program);
		int programSuccess;
		glGetProgramiv(program, GL_LINK_STATUS, &programSuccess);
		if(!programSuccess)
		{
			char log[512];
			glGetProgramInfoLog(program, 512, NULL, log);
			std::cout << "Error linking shader program: " << log << "\n";
			glDeleteProgram(program);
			return false; // unsuccessful
		}

		// if we get here, shader compilation was successful, assign the shader ID member
		m_shaderProgramID = program;
		return true;
	}

	inline void use() const
	{
		if (m_shaderProgramID)
		{
			glUseProgram(m_shaderProgramID);
		}
		else
		{
			std::cout << "Error: invalid shader cannot be used!\n";
		}
	}

	inline void deleteProgram()
	{
		if (m_shaderProgramID)
		{
			glDeleteProgram(m_shaderProgramID);
			m_shaderProgramID = 0;
		}
		else
		{
			std::cout << "Error: invalid shader cannot be deleted!\n";
		}
	}

	inline void clearComponents()
	{
		m_shaderComponents.clear();
	}

	inline GLint getUniformLocation(const char * uniformName) const
	{
		if (!m_shaderProgramID)
		{
			std::cout << "Error: getUniformLocation called on invalid shader!\n";
			return 0;
		}
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
	GLuint m_shaderProgramID;
};

#endif // SHADERPROGRAM_H