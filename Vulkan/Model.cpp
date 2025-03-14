#include "pch.h"
#include "Model.h"


Model::Model(std::nullptr_t) : material(Material(nullptr)) {};

Model::Model(DContext& context, const std::vector<MaterialComponent> components, const std::string& modelPath, const std::vector<std::string>& materialPaths, glm::mat4 transform)
	:material(Material(nullptr))
{
    loadModel(context, modelPath);
    material = Material(context, components, materialPaths);
    


    InitUniformBuffer(context, transform);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorSetAllocateInfo allocInfo{ DescriptorPool::Pool,*DescriptorSetLayout::Get(DescriptorType::Material) };
        material.descriptorSets.push_back(std::move(vk::raii::DescriptorSets(context.logical, allocInfo).front()));

        vk::DescriptorSetAllocateInfo desallocInfo{ DescriptorPool::Pool,*DescriptorSetLayout::Get(DescriptorType::Model) };
        descriptorSets.push_back(std::move(vk::raii::DescriptorSets(context.logical , desallocInfo).front()));

    }
    InitDescriptorSet(context);
    InitDescriptorSetForModelMatrix(context);
}

Model::Model(DContext& context, const std::vector<MaterialComponent> components, BaseModel modelType, const std::vector<std::string>& materialPaths, glm::mat4 transform)
    : material(nullptr)
{
	switch (modelType) {
	case BaseModel::Sphere:
		GenerateSphere(context, *this);
		break;
	case BaseModel::Box:
		GenerateBox(context, *this);
		break;
	case BaseModel::Square:
		GenerateSquare(context, *this);
		break;
	}
	material = Material(context, components, materialPaths);
	InitUniformBuffer(context, transform);

    for(int i=0;i<MAX_FRAMES_IN_FLIGHT;i++){
         vk::DescriptorSetAllocateInfo allocInfo{ DescriptorPool::Pool,*DescriptorSetLayout::Get(DescriptorType::Material)};
         material.descriptorSets.push_back(std::move(vk::raii::DescriptorSets(context.logical, allocInfo).front()));
         
         vk::DescriptorSetAllocateInfo desallocInfo{ DescriptorPool::Pool,*DescriptorSetLayout::Get(DescriptorType::Model) };
         descriptorSets.push_back(std::move(vk::raii::DescriptorSets(context.logical, desallocInfo).front()));
         
    }
    InitDescriptorSet(context);
    InitDescriptorSetForModelMatrix(context);

}

void Model::Render()
{
}

void Model::InitUniformBuffer(DContext& context,glm::mat4 transform)
{
    VkDeviceSize bufferSize = sizeof(Transform);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        uniformBuffers.push_back(DBuffer(context, bufferSize, vk::BufferUsageFlagBits::eUniformBuffer| vk::BufferUsageFlagBits::eShaderDeviceAddress| vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR, vk::MemoryPropertyFlagBits::eHostVisible| vk::MemoryPropertyFlagBits::eHostCoherent));
        uniformBuffers[i].map(bufferSize, 0);
        this->transform.model = { glm::transpose(transform) };
        memcpy(uniformBuffers[i].mapped, &this->transform, sizeof(transform));

    }
    
}

void Model::processNode(DContext& context, aiNode* node, const aiScene* scene)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        // the node object only contains indices to index the actual objects in the scene. 
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		processMesh(context, mesh, scene);
        
    }
    // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(context, node->mChildren[i], scene);
    }
}

void Model::processMesh(DContext& context, aiMesh* mesh, const aiScene* scene)
{// data to fill
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    glm::vec3 vmin(1000, 1000, 1000);
    glm::vec3 vmax(-1000, -1000, -1000);
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex v;
            vmin.x = glm::min(vmin.x, mesh->mVertices[i].x);
            vmin.y = glm::min(vmin.y, mesh->mVertices[i].x);
            vmin.z = glm::min(vmin.z, mesh->mVertices[i].x);
            vmax.x = glm::max(vmax.x, mesh->mVertices[i].x);
            vmax.y = glm::max(vmax.y, mesh->mVertices[i].x);
            vmax.z = glm::max(vmax.z, mesh->mVertices[i].x);
    }

    float dx = vmax.x - vmin.x, dy = vmax.y - vmin.y, dz = vmax.z - vmin.z;
    float dl = glm::max(glm::max(dx, dy), dz);
    float cx = (vmax.x + vmin.x) * 0.5f, cy = (vmax.y + vmin.y) * 0.5f,
        cz = (vmax.z + vmin.z) * 0.5f;
    // walk through each of the mesh's vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
        // positions
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.pos = vector;
        // normals
        if (mesh->HasNormals())
        {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.normal = glm::normalize(vector);
        }
        // texture coordinates
        if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
        {
            glm::vec2 vec;
            // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
            // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.texCoord = vec;
            // tangent
            vector.x = mesh->mTangents[i].x;
            vector.y = mesh->mTangents[i].y;
            vector.z = mesh->mTangents[i].z;
            vertex.tangent = glm::normalize(vector);
            //// bitangent
            //vector.x = mesh->mBitangents[i].x;
            //vector.y = mesh->mBitangents[i].y;
            //vector.z = mesh->mBitangents[i].z;
            //vertex.Bitangent = vector;
        }
        else
            vertex.texCoord = glm::vec2(0.0f, 0.0f);

        vertices.push_back(vertex);
    }
    // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        // retrieve all indices of the face and store them in the indices vector
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    for (auto& v : vertices) {
        v.pos.x = (v.pos.x - cx) / dl;
        v.pos.y = (v.pos.y - cx) / dl;
        v.pos.z = (v.pos.z - cx) / dl;
    }
    // process materials
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
    // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
    // Same applies to other texture as the following list summarizes:
    // diffuse: texture_diffuseN
    // specular: texture_specularN
    // normal: texture_normalN

    // 1. diffuse maps
    //vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
    //textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    //// 2. specular maps
    //vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
    //textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    //// 3. normal maps
    //std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
    //textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    //// 4. height maps
    //std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
    //textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

    // return a mesh object created from the extracted mesh data
	meshes.push_back(Mesh(context, vertices, indices));
}


void Model::InitDescriptorSet(DContext& context)
{
    for (size_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++) {
        std::vector<vk::DescriptorImageInfo> imageInfos(static_cast<uint32_t>(MaterialComponent::END));
        for (int i = 0; i<static_cast<int>(MaterialComponent::END);i++) {
            auto& imageInfo = imageInfos[i];
            imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            if (!material.hasComponent(i)) {
                imageInfo.imageView = *Material::dummy.view;
                imageInfo.sampler = Sampler::Get(SamplerMipMapType::Low);
            }
            else {
                imageInfo.imageView = *material.Get(i).view;
                imageInfo.sampler = Sampler::Get(SamplerMipMapType::High);
            }
        }
        vk::WriteDescriptorSet descriptorWrite{};
        descriptorWrite.dstSet = *material.descriptorSets[frame];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        descriptorWrite.descriptorCount = imageInfos.size();
        descriptorWrite.pImageInfo = imageInfos.data();
        
        context.logical.updateDescriptorSets(descriptorWrite, nullptr);
    }
}

void Model::InitDescriptorSetForModelMatrix(DContext& context)
{
    for (size_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++) {
        vk::DescriptorBufferInfo bufferInfo;
        bufferInfo.buffer = *uniformBuffers[frame].buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(Transform);

        vk::WriteDescriptorSet descriptorWrite{};
        descriptorWrite.dstSet = *descriptorSets[frame];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        context.logical.updateDescriptorSets(descriptorWrite, nullptr); 
    }
}

void Model::loadModel(DContext& context, const std::string& modelPath)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(modelPath, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    // check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
    {
        std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        return;
    }
    // retrieve the directory path of the filepath
    //directory = path.substr(0, path.find_last_of('/'));

    // process ASSIMP's root node recursively
    processNode(context, scene->mRootNode, scene);
}



void GenerateSphere(DContext& context, Model& model)
{


    const int slice = 100;
    std::vector<Vertex> vertices;
    const float dTheta = -2 * glm::pi<float>() / (float)(slice);
    const float dPi = -glm::pi<float>() / (float)(slice);

    for (int j = 0; j <= slice; j++) {
        Vertex v;

        glm::vec3 startPoint = glm::rotate(glm::mat4(1.0f), dPi * j, glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(0.0f, -1.0f, 0.0f, 1.0f);

        for (int i = 0; i <= slice; i++) {
            v.pos = glm::rotate(glm::mat4(1.0f), dTheta * float(i), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(startPoint, 1.0f);
            v.normal = glm::normalize(v.pos);
            v.texCoord = glm::vec2(1-float(i) / slice, 1.0f- float(j) / slice);

            glm::vec3 biTangent = glm::vec3(0.0f, 1.0f, 0.0f);

            glm::vec3 normalOrth = v.normal - glm::dot(biTangent, v.normal) * v.normal;
            normalOrth = glm::normalize(normalOrth);

            v.tangent = glm::cross(biTangent, normalOrth);
            v.tangent = glm::normalize(v.tangent);

            vertices.push_back(v);


        }
    }

    std::vector<uint32_t> indices;
    for (int j = 0; j < slice; j++) {
        const int offset = (slice + 1) * j;
        for (int i = 0; i < slice; i++) {
            indices.push_back(offset + i);
            indices.push_back(offset + i + slice + 1);
            indices.push_back(offset + i + 1 + slice + 1);

            indices.push_back(offset + i);
            indices.push_back(offset + i + 1 + slice + 1);
            indices.push_back(offset + i + 1);
        }
    }

    model.meshes.push_back(Mesh(context, vertices, indices));
}

void GenerateSquare(DContext& context, Model& model)
{
    std::vector<Vertex> vertices;
    Vertex v0, v1, v2, v3;

    // Position: Make the quad lie flat in the XZ plane, facing upwards
    v0.pos = glm::vec3(-1.0f, 0.0f, -1.0f);
    v1.pos = glm::vec3(1.0f, 0.0f, -1.0f);
    v2.pos = glm::vec3(1.0f, 0.0f, 1.0f);
    v3.pos = glm::vec3(-1.0f, 0.0f, 1.0f);

    // Normal: Facing upwards (Y+)
    v0.normal = glm::vec3(0.0f, 1.0f, 0.0f);
    v1.normal = glm::vec3(0.0f, 1.0f, 0.0f);
    v2.normal = glm::vec3(0.0f, 1.0f, 0.0f);
    v3.normal = glm::vec3(0.0f, 1.0f, 0.0f);

    // Texture Coordinates (unchanged)
    v0.texCoord = glm::vec2(0.0f, 0.0f);
    v1.texCoord = glm::vec2(1.0f, 0.0f);
    v2.texCoord = glm::vec2(1.0f, 1.0f);
    v3.texCoord = glm::vec2(0.0f, 1.0f);

    std::vector<uint32_t> indices = {
        0,2,1,0,3,2
    
    };
    glm::vec3 tangent0, tangent1, tangent2, tangent3;

    // Calculate tangent for the square's vertices
    glm::vec3 edge1 = v1.pos - v0.pos;
    glm::vec3 edge2 = v2.pos - v0.pos;
    glm::vec2 deltaUV1 = v1.texCoord - v0.texCoord;
    glm::vec2 deltaUV2 = v2.texCoord - v0.texCoord;

    float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);

    tangent0 = f * (deltaUV2.y * edge1 - deltaUV1.y * edge2);
    tangent1 = tangent0;
    tangent2 = tangent0;
    tangent3 = tangent0;

    v0.tangent = tangent0;
    v1.tangent = tangent1;
    v2.tangent = tangent2;
    v3.tangent = tangent3;

    vertices.push_back(v0);
    vertices.push_back(v1);
    vertices.push_back(v2);
    vertices.push_back(v3);
    model.meshes.push_back(Mesh(context, vertices, indices));
}

void GenerateBox(DContext& context, Model& model)
{
    std::vector<Vertex> vertices(24);
    std::vector<uint32_t> indices = {
        0,  1,  2,  0,  2,  3,  // ����
        4,  5,  6,  4,  6,  7,  // �Ʒ���
        8,  9,  10, 8,  10, 11, // �ո�
        12, 13, 14, 12, 14, 15, // �޸�
        16, 17, 18, 16, 18, 19, // ����
        20, 21, 22, 20, 22, 23  // ������
    };

    // Front face
    vertices[0] = { glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f) };
    vertices[1] = { glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f) };
    vertices[2] = { glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f) };
    vertices[3] = { glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f) };

    // Back face
    vertices[4] = { glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1.0f, 0.0f) };
    vertices[5] = { glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0.0f, 0.0f) };
    vertices[6] = { glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0.0f, 1.0f) };
    vertices[7] = { glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1.0f, 1.0f) };

    // Left face
    vertices[8] = { glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f) };
    vertices[9] = { glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f) };
    vertices[10] = { glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f) };
    vertices[11] = { glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f) };

    // Right face
    vertices[12] = { glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f) };
    vertices[13] = { glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f) };
    vertices[14] = { glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f) };
    vertices[15] = { glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f) };

    // Top face
    vertices[16] = { glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f) };
    vertices[17] = { glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.0f) };
    vertices[18] = { glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 1.0f) };
    vertices[19] = { glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 1.0f) };

    // Bottom face
    vertices[20] = { glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.0f, 1.0f) };
    vertices[21] = { glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1.0f, 1.0f) };
    vertices[22] = { glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1.0f, 0.0f) };
    vertices[23] = { glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.0f, 0.0f) };

    for (int i = 0; i < vertices.size(); i += 4) {
        glm::vec3 edge1 = vertices[i + 1].pos - vertices[i].pos;
        glm::vec3 edge2 = vertices[i + 2].pos - vertices[i].pos;
        glm::vec2 deltaUV1 = vertices[i + 1].texCoord - vertices[i].texCoord;
        glm::vec2 deltaUV2 = vertices[i + 2].texCoord - vertices[i].texCoord;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);

        glm::vec3 tangent = f * (deltaUV2.y * edge1 - deltaUV1.y * edge2);
        tangent = glm::normalize(tangent);

        vertices[i].tangent = tangent;
        vertices[i + 1].tangent = tangent;
        vertices[i + 2].tangent = tangent;
        vertices[i + 3].tangent = tangent;
    }

    model.meshes.push_back(Mesh(context, vertices, indices));
}
