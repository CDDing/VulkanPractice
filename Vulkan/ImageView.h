#pragma once
class ImageView
{
public:
    ImageView();
    ImageView(Device& device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, uint32_t layerCount=1);
    operator VkImageView& () {
        return _imageView;
    }
    void destroy(Device& device) {
        vkDestroyImageView(device, _imageView, nullptr);
    }
private:
    VkImageView _imageView;
};

