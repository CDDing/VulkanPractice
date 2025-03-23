#version 450
layout(push_constant) uniform PushConsts{
	bool hasTexture;
	bool hasNormal;
	bool hasRoughness;
	bool hasMetalness;
	bool hasao;
};

layout (set = 1, binding = 0) uniform sampler2D samplers[5];
layout (set = 3, binding = 0) uniform GUIControl{
	bool useNormalMap;
	float roughness;
	float metallic;
}gc;
layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec3 inWorldPos;
layout(location = 4) in vec3 inTangent;


layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outAlbedo;
layout(location = 3) out vec4 outPBR;
vec3 GetNormal(){
	vec3 normalWorld = normalize(inNormal);
	if(hasNormal){
	vec3 T = normalize(inTangent);
	vec3 N = normalize(inNormal);
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(N, T);
	mat3 TBN = mat3(T, B, N);
	vec3 tnorm = TBN * normalize(texture(samplers[1], inUV).xyz * 2.0 - vec3(1.0));
		normalWorld = normalize(tnorm);
	}
	return normalWorld;
}
void main(){
	vec3 albedo = hasTexture ? texture(samplers[0],inUV).rgb : vec3(1,1,1);
	vec3 normal = GetNormal();
	float roughness = hasRoughness ? texture(samplers[2],inUV).r : gc.roughness;
	float metallic = hasMetalness ? texture(samplers[3],inUV).r : gc.metallic;
	float ao = hasao ? texture(samplers[4],inUV).r:1.0f;
	
	outPosition = vec4(inWorldPos,1.0);
	outNormal = gc.useNormalMap ? vec4(GetNormal(),1.0) : vec4(normalize(inNormal),1.0);
	

	outAlbedo = vec4(albedo,1.0);

	outPBR = vec4(roughness,metallic,ao,0);
}