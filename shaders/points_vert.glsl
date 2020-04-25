#version 330 core

layout(location = 0) in vec3 vertexPos;
layout(location = 1) in uvec3 vertexColour;

flat out uvec3 pointColour;
// flat out uint pointIndex;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(vertexPos.x, vertexPos.y, vertexPos.z, 1.0);
	gl_PointSize = 100.0f / gl_Position.w; // shitty size attenuation
	// this might be the draw ID and not the actual index, which might be a problem when using glDrawElements
	pointColour = vertexColour;
	// pointIndex = uint(gl_VertexID);
}
