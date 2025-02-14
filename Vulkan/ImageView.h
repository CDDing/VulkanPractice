#pragma once
class ImageView
{
public:
    ImageView();
    ImageView(std::shared_ptr<Device> device, vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels, uint32_t layerCount=1);
    operator vk::ImageView& () {
        return _imageView;
    }
    void destroy(std::shared_ptr<Device> device) {
        if (_imageView != VK_NULL_HANDLE) device->logical.destroyImageView(_imageView);
    }
private:
    vk::ImageView _imageView = VK_NULL_HANDLE;
};

