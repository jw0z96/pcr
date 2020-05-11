#version 430

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer visibilityBuffer
{
	uint visibilities[];
};

layout(std430, binding = 1) buffer indexBuffer
{
	uint indices[];
};

layout (binding = 0, offset = 0) uniform atomic_uint visibleCount;

uniform uint seed;

float rand(vec2 co)
{
	return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

// bool getBufferBitAtIndex(uint i)
// {
// 	uint element = i / 32; // 32 bits per uint?
// 	uint remainder = i - 32 * element;
// 	return ((visibilities[element] >> remainder) & 1) == 1;
// }

void main()
{
	// get the correct index into the indices buffer, since we invoke 1 shader call per visibility buffer element
	// uint element = 32 * gl_GlobalInvocationID.x;
	uint index = 32 * gl_GlobalInvocationID.x;

	// for (uint i = 0; i < 32; ++i)
	// {
	// 	uint index = element + i;
	// 	if (getBufferBitAtIndex(index))
	// 	{
	// 		indices[atomicCounterIncrement(visibleCount)] = index;
	// 	}
	// }


	uint element = visibilities[gl_GlobalInvocationID.x];
	for (uint i = 0; i < 32; ++i)
	{
		if (((element & 1) == 1)) // || (rand(vec2(seed, element)) > 0.1f))
		{
			indices[atomicCounterIncrement(visibleCount)] = index + i;
			// atomicCounterIncrement(visibleCount);
		}

		element >>= 1;
	}

	visibilities[gl_GlobalInvocationID.x] = 0;
}
