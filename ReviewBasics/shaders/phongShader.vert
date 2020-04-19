#version 450
layout(location = 0) in vec3 inVertexPosition;
layout(location = 1) in vec3 inVertexNormal;

layout(location = 0) out vec3 LightIntensity;

layout(set = 0, binding = 1) uniform LightUniform
{
    vec4 lightPosition;
    vec4 Kd;
    vec4 Ld;
}lu;

layout(set = 0, binding = 0) uniform CameraUniform
{
    mat4 modelViewMat;
    mat4 projectionMat;
    mat4 MVP;
    mat4 NormalMat;
}cu;

void main()
{
    vec3 tnorm = normalize(mat3(cu.NormalMat) * inVertexNormal);
    vec4 camCoords = cu.modelViewMat * vec4(inVertexPosition, 1.0);
    vec3 s = normalize(vec3(lu.lightPosition - camCoords));
    LightIntensity = vec3(lu.Ld) * vec3(lu.Kd) * max(dot(s, tnorm), 0.0);

    gl_Position = cu.MVP * vec4(inVertexPosition, 1.0);
}