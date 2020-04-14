#version 450
#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) out vec4 Frag_Color;

layout(location = 0) in vec2 TexCoords;
layout(location = 1) in vec3 VertexColor;

layout(binding = 1) uniform sampler2D texSampler;

void main()
{
    Frag_Color = texture(texSampler, TexCoords);
}