#version 450
#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoords;

layout(location = 0) out vec2 TexCoords;
layout(location = 1) out vec3 VertexColor;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 proj;
}ubo;

void main()
{
    TexCoords = inTexCoords;
    VertexColor = inColor;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(position, 1.0);
}