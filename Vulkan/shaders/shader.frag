#version 450
layout(binding = 0) uniform UniformBufferObject{
	mat4 model;
	mat4 view;
	mat4 proj;
	vec4 lights[4];
	vec3 camPos;
}ubo;
layout(binding = 1) uniform sampler2D samplers[5];
//0 , Texture Sampler
//1, NormalMap Sampler
//2 , Roughness Sampler
//3, Metalness Sampler
//4 , ao Sampler

layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 v_tangent;
layout(location = 3) in vec3 inWorldPos;
layout(push_constant) uniform PushConsts{
	bool hasTexture;
	bool hasNormal;
	bool hasRoughness;
	bool hasMetalness;
	bool hasao;
};
const float PI = 3.14159265359;

//#define ROUGHNESS_PATTERN 1

vec3 materialcolor()
{
	return texture(samplers[0],fragTexCoord).xyz;
}

// Normal Distribution function --------------------------------------
float D_GGX(float dotNH, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2)/(PI * denom*denom); 
}

// Geometric Shadowing function --------------------------------------
float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;
	float GL = dotNL / (dotNL * (1.0 - k) + k);
	float GV = dotNV / (dotNV * (1.0 - k) + k);
	return GL * GV;
}

// Fresnel function ----------------------------------------------------
vec3 F_Schlick(float cosTheta, float metallic)
{
	vec3 F0 = mix(vec3(0.04), materialcolor(), metallic); // * material.specular
	vec3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0); 
	return F;    
}

// Specular BRDF composition --------------------------------------------

vec3 BRDF(vec3 L, vec3 V, vec3 N, float metallic, float roughness)
{
	// Precalculate vectors and dot products	
	vec3 H = normalize (V + L);
	float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);
	float dotLH = clamp(dot(L, H), 0.0, 1.0);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);

	// Light color fixed
	vec3 lightColor = vec3(1.0);

	vec3 color = vec3(0.0);

	if (dotNL > 0.0)
	{
		float rroughness = max(0.05, roughness);
		// D = Normal distribution (Distribution of the microfacets)
		float D = D_GGX(dotNH, roughness); 
		// G = Geometric shadowing term (Microfacets shadowing)
		float G = G_SchlicksmithGGX(dotNL, dotNV, rroughness);
		// F = Fresnel factor (Reflectance depending on angle of incidence)
		vec3 F = F_Schlick(dotNV, metallic);

		vec3 spec = D * F * G / (4.0 * dotNL * dotNV);

		color += spec * dotNL * lightColor;
	}

	return color;
}
vec3 GetNormal(){
	vec3 normalWorld = v_normal;
	if(hasNormal){
		vec3 normal = texture(samplers[1],fragTexCoord).xyz;
		normal = 2.0*normal -1.0;

		vec3 N = normalWorld;
		vec3 T = normalize(v_tangent - dot(v_tangent,N) * N);
		vec3 B = cross(N,T);

		mat3x3 TBN = mat3x3(T,B,N);
		normalWorld = normalize(TBN*normal);
	}
	return normalWorld;
}
layout(location = 0) out vec4 outColor;
void main(){

	vec3 lightPos = ubo.lights[0].xyz;
	vec3 lightDir = normalize(inWorldPos - lightPos);
	vec3 normal = normalize(v_normal);


	vec3 normalMap = texture(samplers[1],fragTexCoord).rgb;
	normalMap = normalize(normalMap * 2.0 - 1.0);

	vec3 N = normal;
	vec3 T = normalize(v_tangent - dot(v_tangent,N) * N);
	vec3 B = cross(N,T);

	mat3 TBN = mat3x3(T,B,N);


	vec3 normalWorld = normalize(TBN * normalMap);


	float light = max(dot(normalWorld, -lightDir),0.0);
	
	outColor = texture(samplers[0], fragTexCoord);
	
	outColor.rgb *= light;
}