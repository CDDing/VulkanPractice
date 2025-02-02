#pragma once
class Image
{
public:
    Image();
    Image(Device& device, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,uint32_t arrayLayers = 1) ;
    operator VkImage& () {
        return _image;
    }
    void operator=(VkImage& image) {
        _image = image;
    }
    void destroy(Device& device) {
        if(_image != VK_NULL_HANDLE) vkDestroyImage(device, _image, nullptr);
        if(_imageMemory != VK_NULL_HANDLE) vkFreeMemory(device, _imageMemory, nullptr);
    }
    void fillImage(Device& device, void* data, VkDeviceSize size);
    void transitionLayout(Device& device, VkImageLayout newLayout, VkImageAspectFlags aspectFlags);
    void generateMipmaps(Device& device);
    VkDeviceMemory& GetMemory() { return _imageMemory; }

private:
    VkImage _image = VK_NULL_HANDLE;
    VkDeviceMemory _imageMemory = VK_NULL_HANDLE;
    VkFormat _format;
    uint32_t _width;
    uint32_t _height;
    uint32_t _mipLevels;
    VkImageLayout _layout;
};

void copyBufferToImage(Device& device, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

//For Cubemaps

void copyBufferToImageForCubemap(Device& device, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height,VkDeviceSize layerSize);
void generateMipmapsForCubemap(Device& device, VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
void transitionImageLayoutForCubemap(Device& device, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectFlags, uint32_t mipLevels);