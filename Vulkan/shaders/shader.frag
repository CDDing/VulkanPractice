#version 450
layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform sampler2D normalSampler;

layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;
void main(){

	vec3 lightDir = normalize(vec3(1,1,1));
	vec3 normalMap = texture(normalSampler,fragTexCoord).rgb;
	vec3 adjustedNormal = normalize(normalMap * 2.0 - 1.0);
	float light = dot(adjustedNormal, -lightDir);

	

	
	outColor = texture(texSampler, fragTexCoord);

	outColor.rgb *= light;
}