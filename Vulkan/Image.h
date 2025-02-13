#pragma once
class Image
{
public:
    Image();
    Image(std::shared_ptr<Device> device, uint32_t width, uint32_t height, uint32_t mipLevels, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties,uint32_t arrayLayers = 1) ;
    // 복사 생성자
    Image(const Image& other)
        : _device(other._device), _image(other._image), _imageMemory(other._imageMemory), _format(other._format), _width(other._width), _height(other._height), _mipLevels(other._mipLevels), layout(other.layout) {
    }
    Image& operator=(const Image& other) {
        if (this != &other) {
            _device = other._device;
            _image = other._image;
            _imageMemory = other._imageMemory;
            _format = other._format;
            _width = other._width;
            _height = other._height;
            _mipLevels = other._mipLevels;
            layout = other.layout;
        }
        return *this;
    }

    // 이동 생성자
    Image(Image&& other) noexcept
        : _device(std::move(other._device)), _image(other._image), _imageMemory(other._imageMemory), _format(other._format), _width(other._width), _height(other._height), _mipLevels(other._mipLevels), layout(other.layout) {
        other._image = VK_NULL_HANDLE;
        other._imageMemory = VK_NULL_HANDLE;
    }
    Image& operator=(Image&& other) noexcept {
        if (this != &other) {
            if (_image != VK_NULL_HANDLE) {
                _device->logical.destroyImage(_image);
            }
            if (_imageMemory != VK_NULL_HANDLE) {
                _device->logical.freeMemory(_imageMemory);
            }

            _device = std::move(other._device);
            _image = other._image;
            _imageMemory = other._imageMemory;
            _format = other._format;
            _width = other._width;
            _height = other._height;
            _mipLevels = other._mipLevels;
            layout = other.layout;

            other._image = VK_NULL_HANDLE;
            other._imageMemory = VK_NULL_HANDLE;
        }
        return *this;
    }
    
    operator vk::Image& () {
        return _image;
        
    }
    void operator=(vk::Image& image) {
        _image = image;
    }
    void destroy() {
        if (_image != VK_NULL_HANDLE) _device->logical.destroyImage(_image);
        if (_imageMemory != VK_NULL_HANDLE) _device->logical.freeMemory(_imageMemory);
        _image = VK_NULL_HANDLE;
        _imageMemory = VK_NULL_HANDLE;
    }
    void fillImage(void* data, vk::DeviceSize size);
    void transitionLayout(vk::CommandBuffer commandBuffer, vk::ImageLayout newLayout, vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor);
    void generateMipmaps();
    vk::DeviceMemory& GetMemory() { return _imageMemory; }
    vk::ImageLayout layout = vk::ImageLayout::eUndefined;
  
private:
    std::shared_ptr<Device> _device;
    vk::Image _image = VK_NULL_HANDLE;
    vk::DeviceMemory _imageMemory = VK_NULL_HANDLE;
    vk::Format _format;
    uint32_t _width = WIDTH;
    uint32_t _height = HEIGHT;
    uint32_t _mipLevels = 1;
};

void copyBufferToImage(std::shared_ptr<Device> device, Buffer& buffer, Image& image, uint32_t width, uint32_t height);

//For Cubemaps

void copyBufferToImageForCubemap(std::shared_ptr<Device> device, Buffer& buffer, Image& image, uint32_t width, uint32_t height,vk::DeviceSize layerSize);
void generateMipmapsForCubemap(std::shared_ptr<Device> device, Image& image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
void transitionImageLayoutForCubemap(std::shared_ptr<Device> device, Image& image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels);