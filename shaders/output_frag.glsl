#version 430

layout(binding = 0) uniform isampler2D idTexture;
layout(binding = 1) uniform sampler2D depthTexture;
layout(binding = 2) uniform usamplerBuffer colTexture;

layout(std430, binding = 0) buffer visibilityBuffer
{
	uint visibilities[];
};

layout (std430, binding = 1) writeonly buffer indexBuffer
{
	uint indices[];
};

// note: it doesn't matter whether we use atomicCounterIncrement with a bound atomic counter, or atomicAdd with a SSBO,
// the performance is identical

// // the arguments for glDrawElementsIndirect
// layout(std430, binding = 2) buffer ssIndirectCommand{
// 	uint count;
// 	uint primCount;
// 	uint firstIndex;
// 	uint baseVertex;
// 	uint baseInstance;
// };

layout (binding = 0, offset = 0) uniform atomic_uint visibleCount;

// uniform bool progressive;

in vec2 uv;

out vec4 fragColour;

// void setBufferBitAtIndex(uint i)
// {
// 	const uint element = i / 32; // 32 bits per uint?
// 	const uint remainder = i - 32 * element;
// 	atomicOr(visibilities[element], (1 << remainder));
// }

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
		// setBufferBitAtIndex(pointId); // this is expensive
		indices[atomicCounterIncrement(visibleCount)] = pointId; // but this even more so... huge cost
		// indices[atomicAdd(count, 1)] = pointId; // but this even more so... huge cost

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
