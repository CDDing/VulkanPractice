#pragma once
class Image
{
public:
    Image();
    Image(std::shared_ptr<Device> device, uint32_t width, uint32_t height, uint32_t mipLevels, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties,uint32_t arrayLayers = 1) ;
    operator vk::Image& () {
        return _image;
    }
    void operator=(vk::Image& image) {
        _image = image;
    }
    void destroy(std::shared_ptr<Device> device) {
        if (_image != VK_NULL_HANDLE) device->logical.destroyImage(_image);
        if (_imageMemory != VK_NULL_HANDLE) device->logical.freeMemory(_imageMemory);
    }
    void fillImage(std::shared_ptr<Device> device, void* data, vk::DeviceSize size);
    void transitionLayout(std::shared_ptr<Device> device, vk::CommandBuffer commandBuffer, vk::ImageLayout newLayout, vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor);
    void generateMipmaps(std::shared_ptr<Device> device);
    vk::DeviceMemory& GetMemory() { return _imageMemory; }
    vk::ImageLayout layout = vk::ImageLayout::eUndefined;
    vk::Format format;

private:
    vk::Image _image = VK_NULL_HANDLE;
    vk::DeviceMemory _imageMemory = VK_NULL_HANDLE;
    uint32_t _width = WIDTH;
    uint32_t _height = HEIGHT;
    uint32_t _mipLevels = 1;
};

void copyBufferToImage(std::shared_ptr<Device> device, vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);

//For Cubemaps

void copyBufferToImageForCubemap(std::shared_ptr<Device> device, vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height,vk::DeviceSize layerSize);
void generateMipmapsForCubemap(std::shared_ptr<Device> device, vk::Image image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
void transitionImageLayoutForCubemap(std::shared_ptr<Device> device, Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels);