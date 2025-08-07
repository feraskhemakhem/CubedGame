#version 460 core

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec3 in_color;
layout(location = 1) in vec3 in_normal;

layout(binding = 0) uniform sampler2D u_Texture;

void main()
{
	vec3 lightDir = normalize(vec3(0.5, 1, 0));

	float intensity = max(dot(in_normal, lightDir), 0.15);

	vec4 textureSample = texture(u_Texture, vec2(0, 0));

	out_color = vec4(vec3(intensity) * textureSample.rgb, 1.0);
}