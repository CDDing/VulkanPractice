#version 450


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;


layout(set = 0, binding = 0) uniform UniformBufferObject{
	mat4 view;
	mat4 proj;
	vec4 lights[4];
	vec3 camPos;
}ubo;
layout (location = 0) out vec3 outUVW;

void main() 
{
	outUVW = normalize(inPosition);
	mat4 viewMat = mat4(mat3(ubo.view));
	gl_Position = ubo.proj * viewMat * vec4(inPosition.xyz * 40.0, 1.0);
}
