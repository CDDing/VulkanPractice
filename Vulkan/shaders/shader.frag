#version 450
layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;
void main(){

	vec3 lightDir = vec3(1,1,1);
	float light = dot(v_normal, -lightDir);


	
	outColor = texture(texSampler, fragTexCoord);

	outColor.rgb *= light;
}