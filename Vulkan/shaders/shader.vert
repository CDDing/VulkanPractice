#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject{
	mat4 view;
	mat4 proj;
	vec4 lights[4];
	vec3 camPos;
}ubo;
layout (set = 3, binding = 0) uniform Transform{
	mat4 model;
}transform;
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;

layout(location = 0) out vec3 v_Normal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 v_tangent;
layout(location = 3) out vec3 inWorldPos;

void main(){
	gl_Position = ubo.proj * ubo.view * transform.model * vec4(inPosition,1.0);
	fragTexCoord = inTexCoord;
	inWorldPos = vec3(transform.model * vec4(inPosition,1.0)).xyz;
	mat4 worldInverseTranspose = transpose(inverse(transform.model));
	v_Normal = (worldInverseTranspose * vec4(inNormal,0.0)).xyz;
	
	v_tangent = inTangent;
}