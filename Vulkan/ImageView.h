#pragma once
class ImageView
{
public:
    ImageView();
    ImageView(Device* device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
    VkImageView& Get() { return _imageView; }
private:
    VkImageView _imageView;
};

