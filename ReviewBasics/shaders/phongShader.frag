#version 450
layout(location = 0) out vec4 Frag_Color;
layout(location = 0) in vec3 LightIntensity;

void main()
{
    Frag_Color = vec4(LightIntensity, 1.0);
}