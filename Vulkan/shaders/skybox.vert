#version 450


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;


layout(binding = 0) uniform UniformBufferObject{
	mat4 model;
	mat4 view;
	mat4 proj;
}ubo;

layout (location = 0) out vec3 outUVW;

void main() 
{
	outUVW = normalize(inPosition);
	// Convert cubemap coordinates into Vulkan coordinate space
	//outUVW.xy *= -1.0;
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition,1.0);
}
