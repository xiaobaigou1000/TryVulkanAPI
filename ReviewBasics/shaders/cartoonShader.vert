#version 450
layout(location = 0) in vec3 inVertexPosition;
layout(location = 1) in vec3 inVertexNormal;

layout(location = 0) out vec3 vertexNormal;
layout(location = 1) out vec3 cameraCoords;

layout(set = 0, binding = 0) uniform CameraUniform
{
    mat4 modelViewMat;
    mat4 projectionMat;
    mat4 MVP;
    mat4 NormalMat;
}cu;

void main()
{
    vec4 camCoords = cu.modelViewMat * vec4(inVertexPosition, 1.0);
    cameraCoords = camCoords.xyz;
    vertexNormal = normalize(mat3(cu.NormalMat) * inVertexNormal);
    gl_Position = cu.MVP * vec4(inVertexPosition, 1.0);
}