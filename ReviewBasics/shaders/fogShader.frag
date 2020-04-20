#version 450
//can use with cartoonShader.vert

layout(location = 0) out vec4 Frag_Color;

layout(location = 0) in vec3 vertexNormal;
layout(location = 1) in vec3 cameraCoords;

layout(set = 0, binding = 1) uniform LightUniform
{
    vec4 lightPosition;
    vec4 Kd;
    vec4 Ld;
    vec4 Ka;
    vec4 fogColor;
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
    vec3 LightIntensity = vec3(lu.Ka) + vec3(lu.Ld) * vec3(lu.Kd) * sDotN;
    vec3 color = LightIntensity;

    float fogFactor = (1.0 - gl_FragCoord.z) / 0.3;
    fogFactor = max(fogFactor, 0.0);
    vec3 result = mix(vec3(lu.fogColor), color, fogFactor);
    Frag_Color = vec4(result, 1.0);
}