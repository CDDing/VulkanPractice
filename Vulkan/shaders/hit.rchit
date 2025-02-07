#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference2 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_scalar_block_layout : require
#include "common.glsl"
layout(location = 0) rayPayloadInEXT vec3 hitValue;
layout(location = 2) rayPayloadEXT bool shadowed;
hitAttributeEXT vec2 attribs;

struct GeometryNode {
	uint64_t vertexBufferDeviceAddress;
	uint64_t indexBufferDeviceAddress;
};

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;

layout(binding = 2, set = 0) uniform UniformBufferObject{
	mat4 view;
	mat4 proj;
	vec4 lights[4];
	vec3 camPos;
}ubo;

layout(binding = 3, set = 0) uniform samplerCube samplerCubeMap[3];
layout(binding = 4, set = 0) uniform sampler2D brdfsampler;
layout(binding = 5, set = 0) buffer GeometryNodes { GeometryNode nodes[]; } geometryNodes;

layout(binding = 6, set = 0) uniform sampler2D textures[];

layout (binding = 7,set = 0) uniform GUIControl{
	bool useNormalMap;
	float roughness;
	float metallic;
}gc;


struct Vertex
{
  vec3 pos;
  float padding1;
  
  vec3 normal;
  float padding2;

  vec2 uv;
  vec2 padding3;
  
  vec3 tangent;
  float padding4;
  
  vec4 color;
};

struct Triangle {
	Vertex vertices[3];
	vec3 normal;
	vec2 uv;
};
layout(buffer_reference, scalar) buffer Vertices {Vertex v[];};
layout(buffer_reference, scalar) buffer Indices {uint i[];};

const int MaterialCnt = 5;
vec3 GetValueFromMaterial(int idx, vec2 uv) {
	vec3 result = texture(textures[idx],uv).xyz;
	ivec2 mapSize = textureSize(textures[idx],0);

	//Albedo
	if(idx%MaterialCnt==0){
	}
	else if(idx%MaterialCnt == 1){
	
	}
	else if(idx%MaterialCnt == 2){
		if(mapSize.x == 1 && mapSize.y == 1){
			return vec3(gc.roughness);
		}
	}
	else if(idx%MaterialCnt == 3){
		//metallic

		if(mapSize.x == 1 && mapSize.y == 1){
			return vec3(gc.metallic);
		}
	}
	else if(idx% MaterialCnt == 4){
		//AO

		if(mapSize.x == 1 && mapSize.y == 1){
			return vec3(1);
		}
	}

	return result;
}

const float PI = 3.14159265359;
const vec3 Fdielectric = vec3(0.04); 
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
    vec3 specularIrradiance = textureLod(samplerCubeMap[2],reflection,roughness * 9.0f).rgb;
    
    vec3 F0 = mix(vec3(Fdielectric), albedo, metallic);

    return (F0 * specularBRDF.r + specularBRDF.g) * specularIrradiance;

}
vec3 AmbientLightingByIBL(vec3 albedo, vec3 normalW, vec3 pixelToEye, float ao, float metallic, float roughness){
	vec3 diffuseIBL = DiffuseIBL(albedo, normalW, pixelToEye, metallic);
    vec3 specularIBL = SpecularIBL(albedo, normalW, pixelToEye, metallic, roughness);
    
    return (diffuseIBL + specularIBL) * ao;
}
void main()
{
	GeometryNode geometryNode = geometryNodes.nodes[gl_InstanceCustomIndexEXT];
	Indices indices = Indices(geometryNode.indexBufferDeviceAddress);
	Vertices vertices = Vertices(geometryNode.vertexBufferDeviceAddress);

	int triIndex = gl_PrimitiveID * 3;
	uint index0 = indices.i[triIndex];
	uint index1 = indices.i[triIndex + 1];
	uint index2 = indices.i[triIndex + 2];
	
	Vertex v0 = vertices.v[index0];
	Vertex v1 = vertices.v[index1];
	Vertex v2 = vertices.v[index2];

	// Calculate values at barycentric coordinates
	
	int materialdx =  gl_InstanceCustomIndexEXT * 5;
	
	vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
	const vec3 pos = v0.pos * barycentricCoords.x + v1.pos * barycentricCoords.y + v2.pos * barycentricCoords.z;
	const vec3 worldPos = vec3(gl_ObjectToWorldEXT * vec4(pos,1.0));
	const vec2 uv = v0.uv * barycentricCoords.x + v1.uv * barycentricCoords.y + v2.uv * barycentricCoords.z;
	
	vec3 albedo = GetValueFromMaterial(materialdx + 0,uv);
	vec3 normal = normalize(v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z);
	normal = normalize(vec3(normal * gl_WorldToObjectEXT));
	float roughness  = GetValueFromMaterial(materialdx + 2,uv).r;
	float metallic = GetValueFromMaterial(materialdx + 3,uv).r;
	float ao = GetValueFromMaterial(materialdx + 4,uv).r;

	vec3 hitPoint = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;

	vec3 pixelToEye = normalize(gl_WorldRayOriginEXT - worldPos);

	vec3 ambientLight = AmbientLightingByIBL(albedo,normal,pixelToEye,ao,metallic,roughness);
	
	vec3 directLight = vec3(0);
	
	for(int i =0 ;i < 1;i++){
		vec3 lightPos = ubo.lights[0].xyz;
		vec3 lightVec = normalize(lightPos - worldPos);
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
	
	
	hitValue = vec3(ToSRGB(ambientLight + directLight));

	vec3 lightVector = normalize(ubo.lights[0].xyz) - worldPos;
	
	float tmin = 0.001;
	float tmax = 10000.0;
	vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT + 0.001 * normal;
	shadowed = true;  
	// Trace shadow ray and offset indices to match shadow hit/miss shader group indices
	traceRayEXT(topLevelAS, 
	gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT
	, 0xFF, 0, 0, 1, origin, tmin, lightVector, tmax, 2);
	if (shadowed) {
		//hitValue *= 0.3;
	}
}
