#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;

layout(location = 0) out vec3 out_color;
layout(location = 1) out vec3 out_normal;

layout(push_constant) uniform PushConstants
{
	mat4 ViewProjection;
    mat4 Transform;
} u_PushConstants;

void main()
{
    gl_Position = u_PushConstants.ViewProjection 
                * u_PushConstants.Transform 
                * vec4(a_Position, 1.0);
    // mat3 of transform to ignore translation and focus on rotation
    // transpose and inverse in case object does not scale uniformly
    // normalize to considering scaling
    out_normal = normalize(transpose(inverse(mat3(u_PushConstants.Transform))) * a_Normal);
    out_color = a_Normal * 0.5 + 0.5;
}
