#version 430

layout (local_size_x = 1) in;

layout(binding = 0) uniform isampler2D idTexture;
layout(binding = 1) uniform sampler2D depthTexture;

layout(std430, binding = 0) buffer visibilityBuffer
{
	uint visibilities[];
};

layout (std430, binding = 1) writeonly buffer indexBuffer
{
	uint indices[];
};

uniform uint groupCount;
uniform uint numElements;

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
	// get the correct index into the indices buffer, according to the invocation, group count, and group index
	uint computeIndex = gl_GlobalInvocationID.x * groupCount;
	for (uint group = 0; group < groupCount; ++group, ++computeIndex)
	{
		// careful not to overflow
		if (computeIndex > numElements)
		{
			return;
		}

		const uint indicesIndex = 32 * computeIndex;
		uint element = visibilities[computeIndex];
		for (uint i = 0; i < 32; ++i)
		{
			if (((element & 1) == 1)) // || (rand(vec2(seed, element)) > 0.1f))
			{
				indices[atomicCounterIncrement(visibleCount)] = indicesIndex + i;
			}

			element >>= 1;
		}

		visibilities[computeIndex] = 0;
	}

	// // get the correct index into the indices buffer, since we invoke 1 shader call per visibility buffer element
	// const uint index = 32 * gl_GlobalInvocationID.x;

	// uint element = visibilities[gl_GlobalInvocationID.x];
	// for (uint i = 0; i < 32; ++i)
	// {
	// 	if (((element & 1) == 1)) // || (rand(vec2(seed, element)) > 0.1f))
	// 	{
	// 		indices[atomicCounterIncrement(visibleCount)] = index + i;
	// 	}

	// 	element >>= 1;
	// }

	// visibilities[gl_GlobalInvocationID.x] = 0;
}
