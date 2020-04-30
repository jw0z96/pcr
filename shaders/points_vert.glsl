#version 330 core

layout(location = 0) in vec3 vertexPos;
layout(location = 1) in uvec3 vertexColour;

// flat out uvec3 pointColour;
flat out int pointIndex;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

const float PI =  3.14159265;

vec4 barrel_distort(vec4 p)
{
	vec2 v = p.xy / p.w;

	// Convert to polar coords:
	float radius = length(v);

	if (radius > 0)
	{
		float theta = atan(v.y,v.x);

		// Distort:
		float distortExponent = 0.1f;
		radius = pow(radius, distortExponent);

		// Convert back to Cartesian:
		v.x = radius * cos(theta);
		v.y = radius * sin(theta);
		p.xy = v.xy * p.w;
	}

	return p;
}

vec4 dome_distort(vec4 p)
{
	vec4 newP = p;

	// Convert to polar coords:
	float radius = length(p.xy);

	if (radius > 0)
	{
		float phi = 10.0f * atan(radius, -newP.z);
		newP.xy *= (phi / (PI * 0.5f)) / radius;
	}

	return mix(p, newP, min(length(p.xy/p.w), 1.0f));
	// return newP;
}

void main()
{
	vec4 transformedPos = projection * view * model * vec4(vertexPos.x, vertexPos.y, vertexPos.z, 1.0);
	// gl_Position = dome_distort(transformedPos);
	gl_Position = transformedPos;
	// gl_Position = vec4(0.5f, 0.5f, 0.0f, 1.0f);
	// gl_PointSize = 10.0f;
	gl_PointSize = 100.0f / gl_Position.w; // shitty size attenuation
	// this might be the draw ID and not the actual index, which might be a problem when using glDrawElements
	// pointColour = vertexColour;
	pointIndex = gl_VertexID;
}
