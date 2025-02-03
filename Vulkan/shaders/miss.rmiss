#version 460
#extension GL_EXT_ray_tracing : enable

#include "common.glsl"
layout(location = 0) rayPayloadInEXT vec3 hitValue;

layout (set = 0, binding = 3) uniform samplerCube samplerCubeMap[3];
layout (set = 0, binding = 4) uniform sampler2D brdfsampler;
void main()
{
    vec3 rayDir = normalize(gl_WorldRayDirectionEXT);
    vec3 skyColor = texture(samplerCubeMap[0],rayDir).rgb;
    hitValue = ToSRGB(skyColor);
}