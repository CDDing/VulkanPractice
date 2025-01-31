#version 450
#include "common.glsl"
layout (set = 1, binding = 0) uniform samplerCube samplerCubeMap[3];
layout (set = 1, binding = 1) uniform sampler2D brdfsampler;
layout (location = 0) in vec3 inUVW;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	//outFragColor = vec4(1.0,0.0,0.0,1.0);

	outFragColor = texture(samplerCubeMap[0], inUVW);
	outFragColor = vec4(ToSRGB(outFragColor.xyz),outFragColor.w);
}