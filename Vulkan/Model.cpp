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

void Model::Render()
{
}

void Model::deleteModel(Device& device)
{
    for (auto& mesh : meshes) {
        mesh->deleteMesh(device);
    }

    for (auto& image : images) {
        vkDestroySampler(device.Get(), image.sampler.Get(), nullptr);
        vkDestroyImageView(device.Get(), image.imageView.Get(), nullptr);
        vkDestroyImage(device.Get(), image.image.Get(), nullptr);
        vkFreeMemory(device.Get(), image.image.GetMemory(), nullptr);
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
    std::vector<Texture> textures;

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
    return Mesh(device, vertices, indices, textures);
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

    Mesh mesh = Mesh(device, vertices, indices, {});
    model.meshes.push_back(std::make_shared<Mesh>(mesh));
}