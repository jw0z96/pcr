#version 430

layout(binding = 0) uniform isampler2D idTexture;
layout(binding = 1) uniform sampler2D depthTexture;
layout(binding = 2) uniform usamplerBuffer colTexture;

layout(std430, binding = 0) buffer visibilityBuffer
{
	uint visibilities[];
};

// uniform bool progressive;

in vec2 uv;

out vec4 fragColour;

void setBufferBitAtIndex(uint i)
{
	const uint element = i / 32; // 32 bits per uint?
	const uint remainder = i - 32 * element;
	atomicOr(visibilities[element], (1 << remainder));
}

// bool getBufferBitAtIndex(uint i)
// {
// 	uint element = i / 32; // 32 bits per uint?
// 	uint remainder = i - 32 * element;
// 	return ((visibilities[element] >> remainder) & 1) == 1;
// }

void main()
{
	// check whether the value was set correctly
	// if (getBufferBitAtIndex(359600))
	// {
	// 	setBufferBitAtIndex(0);
	// }
	// setBufferBitAtIndex(359600);

	// setBufferBitAtIndex(0);
	// setBufferBitAtIndex(1);

	const float depth = texture(depthTexture, uv).r;
	if (depth < 1.0f)
	{
		const int pointId = texture(idTexture, uv).r;
		setBufferBitAtIndex(pointId); // this costs 5ms a frame
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
		fragColour = vec4(uv.x, uv.y, 0.0f, 1.0f);
	}
}
