#version 450
layout(set = 0, binding = 0) uniform UniformBufferObject{
	mat4 view;
	mat4 proj;
	vec4 lights[4];
	vec3 camPos;
}ubo;

layout (set = 1, binding = 0) uniform samplerCube samplerCubeMap[3];
layout (set = 1, binding = 1) uniform sampler2D brdfsampler;
layout (set = 2, binding = 0) uniform sampler2D samplers[6];

//0 , Texture Sampler
//1, NormalMap Sampler
//2 , Roughness Sampler
//3, Metalness Sampler
//4 , ao Sampler

layout (location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;
const vec3 Fdielectric = vec3(0.04,0.04,0.04); 
vec3 SchlickFresnel(vec3 F0, float NdotH)
{
    vec3 result = F0;
    result += (1 - F0) * pow(2, (-5.55473 * NdotH - 6.98316) * NdotH);
    return result;
}
float NdfGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    
    float result = a * a;
    result /= PI * pow(((NdotH * NdotH) * (a * a - 1)) + 1, 2);
    
    return result;
}

float SchlickGGX(float NdotI, float NdotO, float roughness)
{
    float k = pow((roughness * roughness + 1), 2) / 8.f;
    float g1I = NdotI / (NdotI * (1 - k) + k);
    float g1O = NdotO / (NdotO * (1 - k) + k);
    return g1I * g1O;
}
vec3 DiffuseIBL(vec3 albedo, vec3 normalWorld, vec3 pixelToEye,
                  float metallic)
{
    vec3 F0 = mix(Fdielectric, albedo, metallic);
    vec3 F = SchlickFresnel(F0, max(0.0, dot(normalWorld, pixelToEye)));
    vec3 kd = mix(1.0 - F, vec3(0.0), metallic);
    
	vec3 irradiance = texture(samplerCubeMap[1],normalWorld).rgb;
    
    return kd * albedo * irradiance;
}

vec3 SpecularIBL(vec3 albedo, vec3 normalWorld, vec3 pixelToEye,
                   float metallic, float roughness)
{
    float NdotO = max(0.0, dot(normalWorld, pixelToEye));
    NdotO /= length(normalWorld) * length(pixelToEye);

	vec2 specularBRDF = texture(brdfsampler,vec2(dot(normalWorld,pixelToEye),1.0-roughness)).rg;
    vec3 reflection = reflect(-pixelToEye, normalize(normalWorld));
    vec3 specularIrradiance = textureLod(samplerCubeMap[2],reflection,roughness * 5.0f).rgb;
    
    vec3 F0 = mix(vec3(Fdielectric), albedo, metallic);

    return (F0 * specularBRDF.r + specularBRDF.g) * specularIrradiance;

}
vec3 AmbientLightingByIBL(vec3 albedo, vec3 normalW, vec3 pixelToEye, float ao, float metallic, float roughness){
	vec3 diffuseIBL = DiffuseIBL(albedo, normalW, pixelToEye, metallic);
    vec3 specularIBL = SpecularIBL(albedo, normalW, pixelToEye, metallic, roughness);
    
    return (diffuseIBL + specularIBL) * ao;
}
void main(){
	// Get G-Buffer values
	vec3 fragPos = texture(samplers[0], inUV).rgb;
	vec3 pixelToEye = normalize(ubo.camPos - fragPos);

	vec3 normal = texture(samplers[1], inUV).rgb;
	vec3 albedo = texture(samplers[2], inUV).rgb;
	float roughness = texture(samplers[3],inUV).r;
	float metallic = texture(samplers[4],inUV).r;
	float ao = texture(samplers[5],inUV).r;
	vec3 ambientLight = AmbientLightingByIBL(albedo,normal,pixelToEye,ao,metallic,roughness);
	
	if(fragPos.x == 0&& fragPos.y==0&&fragPos.z==0){
		discard;
	}
	vec3 directLight;


	for(int i =0 ;i < 1;i++){
		vec3 lightPos = ubo.lights[0].xyz;
		vec3 lightVec = lightPos - fragPos;
		vec3 halfWay = normalize(pixelToEye + lightVec);

		float NdotI = max(0.0,dot(normal,lightVec));
		float NdotH = max(0.0, dot(normal,halfWay));
		float NdotO = max(0.0,dot(normal,pixelToEye));

		vec3 F0 = mix(Fdielectric,albedo,metallic);
		vec3 F = SchlickFresnel(F0, max(0.0,dot(halfWay,pixelToEye)));
		vec3 kd = mix(vec3(1,1,1)- F, vec3(0,0,0),metallic);
		vec3 diffuseBRDF = kd * albedo;

		float D = NdfGGX(NdotH, roughness);
		vec3 G = vec3(SchlickGGX(NdotI,NdotO,roughness));

		vec3 specularBRDF = (F*D*G) / max(1e-5,4.0*NdotI*NdotO);

		vec3 radiance = vec3(1.0);
        vec3 irradiance = radiance * clamp(20.0 - length(lightVec) / (20.0 - 0.0), 0.0, 1.0);
		
		directLight = (diffuseBRDF + specularBRDF) * irradiance * NdotI;
		
	}
	
	
	
	
	
	
	
	
	
	
	outColor = vec4(ambientLight + directLight,1.0);
	
	
	
	outColor = clamp(outColor,0,1000);
}