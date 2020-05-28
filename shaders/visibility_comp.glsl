#version 430

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0) uniform isampler2D idTexture;
layout(binding = 1) uniform sampler2D depthTexture;

// layout(std430, binding = 0) buffer visibilityBuffer
// {
// 	uint visibilities[];
// };

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

	const ivec2 uv = ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);
	const float depth = texelFetch(depthTexture, uv, 0).r;
	if (depth < 1.0f)
	{
		// setBufferBitAtIndex(texelFetch(idTexture, uv, 0).r); // this is expensive
		indices[atomicCounterIncrement(visibleCount)] = texelFetch(idTexture, uv, 0).r; // but this even more so... huge cost
	}
}
