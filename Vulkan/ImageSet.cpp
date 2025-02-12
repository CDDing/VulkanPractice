#include "pch.h"
#include "ImageSet.h"

ImageSet::ImageSet(Device& device, uint32_t width, uint32_t height, uint32_t mipLevels, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::ImageAspectFlags aspectFlags, uint32_t arrayLayers)
{
	image = Image(device, width, height, mipLevels, format, tiling, usage, properties, arrayLayers);
	imageView = ImageView(device, image, format, aspectFlags, mipLevels, arrayLayers);
}
