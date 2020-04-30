#version 430

layout(binding = 0) uniform isampler2D idTexture;
layout(binding = 1) uniform sampler2D depthTexture;
layout(binding = 2) uniform usamplerBuffer colTexture;

in vec2 uv;

out vec4 fragColour;

void main()
{
	int colId = texture(idTexture, uv).r * 3;
	float r = texelFetch(colTexture, colId + 0).r / 255.0f;
	float g = texelFetch(colTexture, colId + 1).r / 255.0f;
	float b = texelFetch(colTexture, colId + 2).r / 255.0f;

	float depth = texture(depthTexture, uv).r;
	float fog = 1.0f - pow(depth, 10.0f);
	// float fog = texture(depthTexture, uv).r;

	// fragColour = vec4(r, g, b, 1.0f);
	fragColour = vec4(r * fog, g * fog, b * fog, 1.0f);
	// fragColour = vec4(fog, fog, fog, 1.0f);
}
