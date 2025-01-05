#pragma once
class Image
{
public:
    Image();
    Image(Device& device, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,uint32_t arrayLayers = 1);
    VkImage& Get() { return _image; }
    VkDeviceMemory& GetMemory() { return _imageMemory; }
private:
    VkImage _image;
    VkDeviceMemory _imageMemory;
};

void copyBufferToImage(Device& device, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
void generateMipmaps(Device& device, VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
void transitionImageLayout(Device& device, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
void copyBufferToImageForCubemap(Device& device, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
void generateMipmapsForCubemap(Device& device, VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
void transitionImageLayoutForCubemap(Device& device, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectFlags, uint32_t mipLevels);