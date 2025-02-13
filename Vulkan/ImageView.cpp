#include "pch.h"
#include "ImageView.h"

ImageView::ImageView()
{
}

ImageView::ImageView(std::shared_ptr<Device> device, vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels,uint32_t layerCount) : _device(device)
{
    vk::ImageViewCreateInfo viewInfo{};
    viewInfo.image = image;
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.layerCount = layerCount;
    viewInfo.subresourceRange.levelCount = mipLevels;
    if (layerCount == 6) {

        viewInfo.viewType = vk::ImageViewType::eCube;
    }

    _imageView = device->logical.createImageView(viewInfo);
}
