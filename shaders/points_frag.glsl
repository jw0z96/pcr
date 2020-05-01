#version 330 core
// #version 430

// layout(std430, binding = 0) buffer pointColourBuffer
// {
// 	uvec4 pointColours[];
// };

// uniform uvec3 pointColours[];

// uniform usamplerBuffer colTexture;

// flat in uvec3 pointColour;
flat in int pointIndex;

// out vec4 fragColour;
layout(location = 0) out int fragIndex;
// out int fragIndex;

void main()
{
	vec2 cxy = 2.0f * gl_PointCoord - 1.0f;
	// float delta = 0.5f;
	// // // float alpha = 1.0f - smoothstep(1.0f - delta, 1.0f + delta, length(gl_PointCoord - vec2(0.5)));
	// float alpha = 1.0f - smoothstep(1.0f - delta, 1.0f + delta, dot(cxy, cxy));
	if (dot(cxy, cxy) > 1.0)
		discard;
	// fragColour = vec4(col, alpha);

	fragIndex = pointIndex;
}
