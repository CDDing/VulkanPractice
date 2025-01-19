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

layout (set = 2, binding = 0) uniform Transform{
	mat4 model;
}transform;
layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outUV;
layout(location = 2) out vec3 outColor;
layout(location = 3) out vec3 outWorldPos;
layout(location = 4) out vec3 outTangent;
void main(){
	outWorldPos = vec3(transform.model * vec4(inPosition,1.0));
	gl_Position  = ubo.proj * ubo.view * vec4(outWorldPos,1.0);
	outUV = inTexCoord;

	//outColor = inColor;

	mat3 mNormal= transpose(inverse(mat3(transform.model)));
	outNormal = mNormal * normalize(inNormal);
	outTangent = mNormal * normalize(inTangent);
}