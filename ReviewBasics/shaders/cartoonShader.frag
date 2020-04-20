#version 450
layout(location = 0) out vec4 Frag_Color;

layout(location = 0) in vec3 vertexNormal;
layout(location = 1) in vec3 cameraCoords;

layout(set = 0, binding = 1) uniform LightUniform
{
    vec4 lightPosition;
    vec4 Kd;
    vec4 Ld;
    vec4 Ka;
}lu;

layout(push_constant) uniform ConstantBlock
{
    float levels;
    float scaleFactor;
}pushConsts;

void main()
{
    vec3 s = normalize(vec3(lu.lightPosition) - cameraCoords);
    float sDotN = max(dot(s, vertexNormal), 0.0);
    vec3 LightIntensity = vec3(lu.Ld) * vec3(lu.Kd) * floor(sDotN * pushConsts.levels) * pushConsts.scaleFactor;
    Frag_Color = lu.Ka + vec4(LightIntensity, 1.0);
}