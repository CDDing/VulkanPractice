#include "pch.h"
#include "Model.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
Model::Model(Device& device, const std::string& modelPath)
{
    loadModel(device, modelPath);
}

Model::Model(Device& device, const std::string& modelPath, const std::string& texturePath)
{
    loadModel(device, modelPath);
    loadImage(device, texturePath);
}

Model::Model(Device& device, const std::string& modelPath, const std::string& texturePath, const std::string& normalMapPath)
{
    loadModel(device, modelPath);
    loadImage(device, texturePath);
    loadImage(device, normalMapPath);
}

Model makeSphere(Device& device, const float& scale, const std::string& texturePath)
{
    Model model;
    GenerateSphere(device, model,scale);
    model.loadImage(device,texturePath);
    return model;
}

Model makeSphere(Device& device, const float& scale, const std::string& texturePath, const std::string& normalMapPath)
{
    Model model;
    GenerateSphere(device,model,scale);
    model.loadImage(device, texturePath);
    model.loadImage(device, normalMapPath);
    return model;
}

Model makeSqaure(Device& device, const float& scale, const std::string& texturePath)
{
    Model model;
    GenerateSquare(device, model, scale);
    model.loadImage(device, texturePath);
    return model;
}

Model makeSqaure(Device& device, const float& scale, const std::string& texturePath, const std::string& normalMapPath)
{
    Model model;
    GenerateSquare(device, model, scale);
    model.loadImage(device, texturePath);
    model.loadImage(device, normalMapPath);
    return model;
}

Model makeBox(Device& device, const float& scale, const std::string& texturePath)
{
    Model model;
    GenerateBox(device, model, scale);
    model.loadImage(device, texturePath);
    return model;
}

Model makeBox(Device& device, const float& scale, const std::string& texturePath, const std::string& normalMapPath)
{
    Model model;
    GenerateBox(device, model, scale);
    model.loadImage(device, texturePath);
    model.loadImage(device, normalMapPath);
    return model;
}

void Model::Render()
{
}

void Model::destroy(Device& device)
{
    for (auto& mesh : meshes) {
        mesh->destroy(device);
    }

    for (auto& image : images) {
        image.destroy(device);
    }


}

void Model::processNode(Device& device, aiNode* node, const aiScene* scene)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        // the node object only contains indices to index the actual objects in the scene. 
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(std::make_shared<Mesh>(processMesh(device, mesh, scene)));
    }
    // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(device, node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(Device& device, aiMesh* mesh, const aiScene* scene)
{// data to fill
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

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
            vertex.normal = vector;
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
            vertex.tangent = vector;
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
    return Mesh(device, vertices, indices);
}


void Model::loadModel(Device& device, const std::string& modelPath)
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
    processNode(device, scene->mRootNode, scene);
}

void Model::loadImage(Device& device, const std::string& filePath)
{
    Texture image;

    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    _mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    Buffer stagingBuffer;

    stagingBuffer = Buffer(device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* data;
    vkMapMemory(device.Get(), stagingBuffer.GetMemory(), 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device.Get(), stagingBuffer.GetMemory());

    stbi_image_free(pixels);
    image.image = Image(device, texWidth, texHeight, _mipLevels, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    transitionImageLayout(device, image.image.Get(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, _mipLevels);
    copyBufferToImage(device,stagingBuffer.Get(), image.image.Get(), static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    //transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

    vkDestroyBuffer(device.Get(), stagingBuffer.Get(), nullptr);
    vkFreeMemory(device.Get(), stagingBuffer.GetMemory(), nullptr);

    generateMipmaps(device,image.image.Get(), VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, _mipLevels);

    image.imageView = ImageView(device, image.image.Get(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, _mipLevels);
    image.sampler = Sampler(device, _mipLevels);

    images.push_back(image);
}
void GenerateSphere(Device& device, Model& model,const float& scale)
{


    const int slice = 50;
    std::vector<Vertex> vertices;
    const float dTheta = 2 * glm::pi<float>() / (float)(slice);
    const float dPi = glm::pi<float>() / (float)(slice);

    for (int j = 0; j <= slice; j++) {
        Vertex v;

        glm::vec3 startPoint = glm::rotate(glm::mat4(1.0f), dPi * j, glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(0.0f, -scale, 0.0f, 1.0f);

        for (int i = 0; i <= slice; i++) {
            v.pos = glm::rotate(glm::mat4(1.0f), dTheta * float(i), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(startPoint, 1.0f);
            v.normal = glm::normalize(v.pos);
            v.texCoord = glm::vec2(float(i) / slice, 1.0f- float(j) / slice);

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
            indices.push_back(offset + i + 1 + slice + 1);
            indices.push_back(offset + i + slice + 1);

            indices.push_back(offset + i);
            indices.push_back(offset + i + 1);
            indices.push_back(offset + i + 1 + slice + 1);
        }
    }

    Mesh mesh = Mesh(device, vertices, indices);
    model.meshes.push_back(std::make_shared<Mesh>(mesh));
}

void GenerateSquare(Device& device, Model& model, const float& scale)
{
    std::vector<Vertex> vertices;
    Vertex v0, v1, v2, v3;
    v0.pos = (glm::vec3(-1.0f, 1.0f, 0.0f) * scale);
    v1.pos=(glm::vec3(1.0f, 1.0f, 0.0f) * scale);
    v2.pos=(glm::vec3(1.0f, -1.0f, 0.0f) * scale);
    v3.pos=(glm::vec3(-1.0f, -1.0f, 0.0f) * scale);
    v0.normal=(glm::vec3(0.0f, 0.0f, -1.0f));
    v1.normal=(glm::vec3(0.0f, 0.0f, -1.0f));
    v2.normal=(glm::vec3(0.0f, 0.0f, -1.0f));
    v3.normal=(glm::vec3(0.0f, 0.0f, -1.0f));
    v0.texCoord = (glm::vec2(0.0f, 0.0f));
    v1.texCoord = (glm::vec2(1.0f, 0.0f));
    v2.texCoord = (glm::vec2(1.0f, 1.0f));
    v3.texCoord = (glm::vec2(0.0f, 1.0f));
    std::vector<uint32_t> indices = {
        0,1,2,0,2,3
    
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
    Mesh mesh = Mesh(device, vertices, indices);
    model.meshes.push_back(std::make_shared<Mesh>(mesh));
}

void GenerateBox(Device& device, Model& model, const float& scale)
{
    std::vector<Vertex> vertices(24);
    std::vector<uint32_t> indices = {
        0,  1,  2,  0,  2,  3,  // À­¸é
        4,  5,  6,  4,  6,  7,  // ¾Æ·§¸é
        8,  9,  10, 8,  10, 11, // ¾Õ¸é
        12, 13, 14, 12, 14, 15, // µÞ¸é
        16, 17, 18, 16, 18, 19, // ¿ÞÂÊ
        20, 21, 22, 20, 22, 23  // ¿À¸¥ÂÊ
    };

    // Front face
    vertices[0] = { glm::vec3(-1.0f, -1.0f, -1.0f) * scale, glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f) };
    vertices[1] = { glm::vec3(-1.0f, 1.0f, -1.0f) * scale, glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f) };
    vertices[2] = { glm::vec3(1.0f, 1.0f, -1.0f) * scale, glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f) };
    vertices[3] = { glm::vec3(1.0f, -1.0f, -1.0f) * scale, glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f) };

    // Back face
    vertices[4] = { glm::vec3(-1.0f, -1.0f, 1.0f) * scale, glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1.0f, 0.0f) };
    vertices[5] = { glm::vec3(1.0f, -1.0f, 1.0f) * scale, glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0.0f, 0.0f) };
    vertices[6] = { glm::vec3(1.0f, 1.0f, 1.0f) * scale, glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0.0f, 1.0f) };
    vertices[7] = { glm::vec3(-1.0f, 1.0f, 1.0f) * scale, glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1.0f, 1.0f) };

    // Left face
    vertices[8] = { glm::vec3(-1.0f, -1.0f, 1.0f) * scale, glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f) };
    vertices[9] = { glm::vec3(-1.0f, 1.0f, 1.0f) * scale, glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f) };
    vertices[10] = { glm::vec3(-1.0f, 1.0f, -1.0f) * scale, glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f) };
    vertices[11] = { glm::vec3(-1.0f, -1.0f, -1.0f) * scale, glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f) };

    // Right face
    vertices[12] = { glm::vec3(1.0f, -1.0f, 1.0f) * scale, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f) };
    vertices[13] = { glm::vec3(1.0f, -1.0f, -1.0f) * scale, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f) };
    vertices[14] = { glm::vec3(1.0f, 1.0f, -1.0f) * scale, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f) };
    vertices[15] = { glm::vec3(1.0f, 1.0f, 1.0f) * scale, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f) };

    // Top face
    vertices[16] = { glm::vec3(-1.0f, 1.0f, -1.0f) * scale, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f) };
    vertices[17] = { glm::vec3(-1.0f, 1.0f, 1.0f) * scale, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.0f) };
    vertices[18] = { glm::vec3(1.0f, 1.0f, 1.0f) * scale, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 1.0f) };
    vertices[19] = { glm::vec3(1.0f, 1.0f, -1.0f) * scale, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 1.0f) };

    // Bottom face
    vertices[20] = { glm::vec3(-1.0f, -1.0f, -1.0f) * scale, glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.0f, 1.0f) };
    vertices[21] = { glm::vec3(1.0f, -1.0f, -1.0f) * scale, glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1.0f, 1.0f) };
    vertices[22] = { glm::vec3(1.0f, -1.0f, 1.0f) * scale, glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1.0f, 0.0f) };
    vertices[23] = { glm::vec3(-1.0f, -1.0f, 1.0f) * scale, glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.0f, 0.0f) };

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

    Mesh mesh = Mesh(device, vertices, indices);
    model.meshes.push_back(std::make_shared<Mesh>(mesh));
}

Model makeSkyBox(Device& device)
{
    Model model;
    GenerateBox(device, model, 0.5f);

    Texture image;

    int width,height, channels;
    uint32_t mipLevels;
    stbi_uc* faceData[6];
    faceData[0] = stbi_load("Resources/textures/Cubemap/right.jpg", &width, &height, &channels, STBI_rgb_alpha);
    faceData[1] = stbi_load("Resources/textures/Cubemap/left.jpg", &width, &height, &channels, STBI_rgb_alpha);
    faceData[2] = stbi_load("Resources/textures/Cubemap/top.jpg", &width, &height, &channels, STBI_rgb_alpha);
    faceData[3] = stbi_load("Resources/textures/Cubemap/bottom.jpg", &width, &height, &channels, STBI_rgb_alpha);
    faceData[4] = stbi_load("Resources/textures/Cubemap/front.jpg", &width, &height, &channels, STBI_rgb_alpha);
    faceData[5] = stbi_load("Resources/textures/Cubemap/back.jpg", &width, &height, &channels, STBI_rgb_alpha); 
    VkDeviceSize imageSize = width * height * channels * 6;
    VkDeviceSize layerSize = width * height * channels;
    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
    //mipLevels = 01;

    Buffer stagingBuffer;
    stagingBuffer = Buffer(device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* data;
    vkMapMemory(device.Get(), stagingBuffer.GetMemory(), 0, imageSize, 0, &data);
    for (int i = 0; i < 6; i++) {
        memcpy((stbi_uc*)data + i*layerSize, faceData[i], static_cast<size_t>(layerSize));

    }
    vkUnmapMemory(device.Get(), stagingBuffer.GetMemory());

    for (int i = 0; i < 6; i++) {
        stbi_image_free(faceData[i]);
    }
    
    image.image = Image(device, width, height, mipLevels, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,6);

    transitionImageLayoutForCubemap(device, image.image.Get(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
    copyBufferToImageForCubemap(device, stagingBuffer.Get(), image.image.Get(), static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    //transitionImageLayoutForCubemap(device, image.image.Get(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

    vkDestroyBuffer(device.Get(), stagingBuffer.Get(), nullptr);
    vkFreeMemory(device.Get(), stagingBuffer.GetMemory(), nullptr);

    generateMipmapsForCubemap(device, image.image.Get(), VK_FORMAT_R8G8B8A8_SRGB, width, height, mipLevels);
    
    
    image.imageView = ImageView(device, image.image.Get(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels,6);
    image.sampler = Sampler(device, mipLevels);

    model.images.push_back(image);

    return model;
}
