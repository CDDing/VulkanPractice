#pragma once
class Image
{
public:
    Image();
    Image(Device* device, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
    VkImage& Get() { return _image; }
    VkDeviceMemory& GetMemory() { return _imageMemory; }
private:
    VkImage _image;
    VkDeviceMemory _imageMemory;
};