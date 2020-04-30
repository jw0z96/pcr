#version 330 core

out vec2 uv;

void main()
{
	// 4 vertex TRIANGLE_STRIP
	// const vec2 positions[4] = vec2[](
	// 	vec2(-1, -1),
	// 	vec2(+1, -1),
	// 	vec2(-1, +1),
	// 	vec2(+1, +1)
	// );

	// const vec2 coords[4] = vec2[](
	// 	vec2(0, 0),
	// 	vec2(1, 0),
	// 	vec2(0, 1),
	// 	vec2(1, 1)
	// );

	// uv = coords[gl_VertexID];
	// gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);

	// 6 vertices TRIANGLES
	float x = float(((uint(gl_VertexID) + 2u) / 3u)%2u);
	float y = float(((uint(gl_VertexID) + 1u) / 3u)%2u);

	gl_Position = vec4(-1.0f + x*2.0f, -1.0f+y*2.0f, 0.0f, 1.0f);
	uv = vec2(x, y);
}
