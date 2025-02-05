#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference2 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_scalar_block_layout : require
layout(location = 0) rayPayloadInEXT vec3 hitValue;
hitAttributeEXT vec2 attribs;

struct GeometryNode {
	uint64_t vertexBufferDeviceAddress;
	uint64_t indexBufferDeviceAddress;
};
layout(binding = 5, set = 0) buffer GeometryNodes { GeometryNode nodes[]; } geometryNodes;

layout(binding = 6, set = 0) uniform sampler2D textures[];



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

void main()
{
	GeometryNode geometryNode = geometryNodes.nodes[gl_GeometryIndexEXT];
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
	
	uint materialdx = gl_GeometryIndexEXT * 5;
	
	vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
	vec2 uv = v0.uv * barycentricCoords.x + v1.uv * barycentricCoords.y + v2.uv * barycentricCoords.z;

	vec3 albedo = texture(textures[materialdx + 0],uv).xyz;

	hitValue = vec3(albedo);
}
