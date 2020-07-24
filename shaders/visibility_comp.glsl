#version 430

// layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
// layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout (local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(binding = 0) uniform isampler2D idTexture;
layout(binding = 1) uniform sampler2D depthTexture;

layout(std430, binding = 0) buffer visibilityBuffer
{
	uint visibilities[];
};

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
	const ivec2 uv = ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);
	const float depth = texelFetch(depthTexture, uv, 0).r;
	if (depth < 1.0f)
	{
		// atomic max this value? so we know how many elementComputeShader to dispatch?
		const int pointId = texelFetch(idTexture, uv, 0).r;
		// could use modf here?
		const uint element = pointId / 32; // 32 bits per uint?
		const uint remainder = pointId - (32 * element);
		atomicOr(visibilities[element], (1 << remainder));
		// setBufferBitAtIndex(texelFetch(idTexture, uv, 0).r); // this is expensive
	}
}
