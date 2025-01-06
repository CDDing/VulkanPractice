#version 450
layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform sampler2D normalSampler;

layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 v_tangent;

layout(push_constant) uniform PushConsts{
	float roughness;
	float metallic;
	float r;
	float g;
	float b;
};

layout(location = 0) out vec4 outColor;
void main(){


	vec3 lightDir = normalize(vec3(1,1,1));
	vec3 normal = normalize(v_normal);

	vec3 normalMap = texture(normalSampler,fragTexCoord).rgb;
	normalMap = normalize(normalMap * 2.0 - 1.0);

	vec3 N = normal;
	vec3 T = normalize(v_tangent - dot(v_tangent,N) * N);
	vec3 B = cross(N,T);

	mat3 TBN = mat3x3(T,B,N);

	vec3 normalWorld = normalize(TBN * normalMap);
	

	float light = max(dot(normalWorld, -lightDir),0.0);
	
	outColor = texture(texSampler, fragTexCoord);
	
	outColor.rgb *= light;
}