#version 430

layout(binding = 0) uniform isampler2D idTexture;
layout(binding = 1) uniform sampler2D depthTexture;
layout(binding = 2) uniform usamplerBuffer colTexture;

layout (std430, binding = 1) writeonly buffer indexBuffer
{
	uint indices[];
};

layout (binding = 0, offset = 0) uniform atomic_uint visibleCount;

in vec2 uv;

out vec4 fragColour;

void main()
{
	const float depth = texture(depthTexture, uv).r;
	if (depth < 1.0f)
	{
		const int pointId = texture(idTexture, uv).r;
		const int colourId = pointId * 3;
		const float r = texelFetch(colTexture, colourId + 0).r / 255.0f;
		const float g = texelFetch(colTexture, colourId + 1).r / 255.0f;
		const float b = texelFetch(colTexture, colourId + 2).r / 255.0f;
		// float fog = 1.0f - pow(depth, 10.0f);
		// fragColour = vec4(r * fog, g * fog, b * fog, 1.0f);
		fragColour = vec4(r, g, b, 1.0f);
	}
	else
	{
		// fragColour = vec4(uv.x, uv.y, 0.0f, 1.0f);
		fragColour = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	}
}
