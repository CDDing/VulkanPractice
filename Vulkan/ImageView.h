#pragma once
class ImageView
{
public:
    ImageView();
    ImageView(Device& device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, uint32_t layerCount=1);
    VkImageView& Get() { return _imageView; }
private:
    VkImageView _imageView;
};

