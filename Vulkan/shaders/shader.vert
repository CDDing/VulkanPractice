#version 450

layout(binding = 0) uniform UniformBufferObject{
	mat4 model;
	mat4 view;
	mat4 proj;
	vec3 camPos;
}ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;

layout(location = 0) out vec3 v_Normal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 v_tangent;
layout(location = 3) out vec3 inWorldPos;

void main(){
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition,1.0);
	fragTexCoord = inTexCoord;
	inWorldPos = vec3(ubo.model * vec4(inPosition,1.0)).xyz;
	mat4 worldInverseTranspose = transpose(inverse(ubo.model));
	v_Normal = (worldInverseTranspose * vec4(inNormal,0.0)).xyz;
	
	v_tangent = inTangent;
}