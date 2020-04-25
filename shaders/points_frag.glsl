#version 330 core

flat in uvec3 pointColour;
// flat in uint pointIndex;

out vec4 fragColour;
// out uint fragIndex;

void main()
{
	vec3 col = pointColour / 255.0f;
	// vec2 cxy = 2.0f * gl_PointCoord - 1.0f;
	// // float delta = 0.5f;
	// // float alpha = 1.0f - smoothstep(1.0f - delta, 1.0f + delta, length(gl_PointCoord - vec2(0.5)));
	// // float alpha = 1.0f - smoothstep(1.0f - delta, 1.0f + delta, dot(cxy, cxy));
	// if (dot(cxy, cxy) > 1.0)
	// 	discard;

	// fragColour = vec4(col, alpha);
	fragColour = vec4(col, 1.0f);
	// fragIndex = pointIndex;
}
