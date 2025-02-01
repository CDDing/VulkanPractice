#include "pch.h"
#include "ImageView.h"

ImageView::ImageView()
{
}

ImageView::ImageView(Device& device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels,uint32_t layerCount)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.layerCount = layerCount;
    viewInfo.subresourceRange.levelCount = mipLevels;
    if (layerCount == 6) {

        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    }
    if (vkCreateImageView(device, &viewInfo, nullptr, &_imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }
}
