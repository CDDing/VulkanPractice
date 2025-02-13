#include "pch.h"
#include "Image.h"

Image::Image()
{
}

Image::Image(std::shared_ptr<Device> device, uint32_t width, uint32_t height, uint32_t mipLevels, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, uint32_t arrayLayer)
    : _width(width), _height(height), _format(format), _mipLevels(mipLevels), _device(device)
{
    vk::ImageCreateInfo imageInfo{ {}, vk::ImageType::e2D,format,
        {static_cast<uint32_t>(width),static_cast<uint32_t>(height),1},mipLevels,
    arrayLayer,  vk::SampleCountFlagBits::e1,
    tiling,usage,vk::SharingMode::eExclusive};

    if (arrayLayer == 6) {
        imageInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;
    }

    _image = device->logical.createImage(imageInfo);

    vk::MemoryRequirements memRequirements = device->logical.getImageMemoryRequirements(_image);
    

	vk::MemoryAllocateInfo allocInfo{memRequirements.size,
        findMemoryType(device->physical, memRequirements.memoryTypeBits, properties)
    };

    _imageMemory = device->logical.allocateMemory(allocInfo);
    device->logical.bindImageMemory(_image, _imageMemory,0);

}

void Image::fillImage(void* data ,vk::DeviceSize size)
{
    Buffer stagingBuffer;

    stagingBuffer = Buffer(_device, size, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible| vk::MemoryPropertyFlagBits::eHostCoherent);

    stagingBuffer.map(size, 0);
    memcpy(stagingBuffer.mapped, data, static_cast<size_t>(size));
    stagingBuffer.unmap();

    vk::CommandBuffer cmdBuf = beginSingleTimeCommands(_device);
    transitionLayout(cmdBuf, 
        vk::ImageLayout::eTransferDstOptimal);
    endSingleTimeCommands(_device, cmdBuf);
    copyBufferToImage(_device, stagingBuffer, *this, static_cast<uint32_t>(_width), static_cast<uint32_t>(_height));

}

void Image::transitionLayout(vk::CommandBuffer commandBuffer, vk::ImageLayout newLayout, vk::ImageAspectFlags aspectFlags)
{

    vk::ImageMemoryBarrier imageMemoryBarrier{};
    imageMemoryBarrier.oldLayout = layout;
    imageMemoryBarrier.newLayout = newLayout;

    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.image = _image;
    imageMemoryBarrier.subresourceRange.aspectMask = aspectFlags;
    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.layerCount = 1;
    imageMemoryBarrier.subresourceRange.levelCount = _mipLevels;
    imageMemoryBarrier.srcAccessMask = (vk::AccessFlagBits)0;
    imageMemoryBarrier.dstAccessMask = (vk::AccessFlagBits)0;

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    switch (layout)
    {
    case vk::ImageLayout::eUndefined:
        // Image layout is undefined (or does not matter)
        // Only valid as initial layout
        // No flags required, listed only for completeness
        imageMemoryBarrier.srcAccessMask = (vk::AccessFlagBits)0;
        break;

    case vk::ImageLayout::ePreinitialized:
        // Image is preinitialized
        // Only valid as initial layout for linear images, preserves memory contents
        // Make sure host writes have been finished
        imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
        break;

    case vk::ImageLayout::eColorAttachmentOptimal:
        // Image is a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        break;

    case vk::ImageLayout::eDepthAttachmentOptimal:
        // Image is a depth/stencil attachment
        // Make sure any writes to the depth/stencil buffer have been finished
        imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        break;

    case vk::ImageLayout::eTransferSrcOptimal:
        // Image is a transfer source
        // Make sure any reads from the image have been finished
        imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        break;

    case vk::ImageLayout::eTransferDstOptimal:
        // Image is a transfer destination
        // Make sure any writes to the image have been finished
        imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        break;

    case vk::ImageLayout::eShaderReadOnlyOptimal:
        // Image is read by a shader
        // Make sure any shader reads from the image have been finished
        imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
        break;
    default:
        // Other source layouts aren't handled (yet)
        break;
    }

    // Target layouts (new)
    // Destination access mask controls the dependency for the new image layout
    switch (newLayout)
    {
    case vk::ImageLayout::eTransferDstOptimal:
        // Image will be used as a transfer destination
        // Make sure any writes to the image have been finished
        imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        break;

    case vk::ImageLayout::eTransferSrcOptimal:
        // Image will be used as a transfer source
        // Make sure any reads from the image have been finished
        imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
        break;

    case vk::ImageLayout::eColorAttachmentOptimal:
        // Image will be used as a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        break;

    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
        // Image layout will be used as a depth/stencil attachment
        // Make sure any writes to depth/stencil buffer have been finished
        imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        break;

    case vk::ImageLayout::eShaderReadOnlyOptimal:
        // Image will be read in a shader (sampler, input attachment)
        // Make sure any writes to the image have been finished
        if (imageMemoryBarrier.srcAccessMask == (vk::AccessFlagBits)0)
        {
            imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eHostWrite| vk::AccessFlagBits::eTransferWrite;
        }
        imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        break;
    default:
        // Other source layouts aren't handled (yet)
        break;
    }

    //TODO
    sourceStage = (vk::PipelineStageFlagBits)65536U;
    destinationStage = (vk::PipelineStageFlagBits)65536U;

    commandBuffer.pipelineBarrier(sourceStage, destinationStage,vk::DependencyFlags(),nullptr,nullptr, imageMemoryBarrier);
    layout = newLayout;
}

void Image::generateMipmaps()
{

    vk::FormatProperties formatProperties = _device->physical.getFormatProperties(_format);

    if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        throw std::runtime_error("texture image format does not linear blitting!");
    }
    vk::CommandBuffer commandBuffer = beginSingleTimeCommands(_device);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = _image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;


    int32_t mipWidth = _width;
    int32_t mipHeight = _height;

    for (uint32_t i = 1; i < _mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0,0,0 };
        blit.srcOffsets[1] = { mipWidth,mipHeight,1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0,0,0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1,mipHeight > 1 ? mipHeight / 2 : 1,1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer, _image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1)mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = _mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    endSingleTimeCommands(_device, commandBuffer);
}

void copyBufferToImage(std::shared_ptr<Device> device, Buffer& buffer, Image& image, uint32_t width, uint32_t height) {
	vk::CommandBuffer commandBuffer = beginSingleTimeCommands(device);

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0,0,0 };
	region.imageExtent = {
		width,height,1 };

    commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, { region });
	endSingleTimeCommands(device, commandBuffer);
}



void copyBufferToImageForCubemap(std::shared_ptr<Device> device, Buffer& buffer, Image& image, uint32_t width, uint32_t height,vk::DeviceSize layerSize)
{
    vk::CommandBuffer commandBuffer = beginSingleTimeCommands(device);

    vk::BufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.layerCount = 1;  // 한 레이어씩 복사

    region.imageOffset = vk::Offset3D{ 0, 0, 0 };
    region.imageExtent = vk::Extent3D{ width, height, 1 };

    // 6개의 레이어에 대해 반복
    for (uint32_t layer = 0; layer < 6; layer++) {
        region.imageSubresource.baseArrayLayer = layer;  // 각 레이어에 대해 baseArrayLayer 설정
        region.bufferOffset = layer * layerSize;
        commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, { region });
    }
    endSingleTimeCommands(device, commandBuffer);
}
void generateMipmapsForCubemap(std::shared_ptr<Device> device, Image& image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
    vk::FormatProperties formatProperties = device->physical.getFormatProperties(imageFormat);
    if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    vk::CommandBuffer commandBuffer = beginSingleTimeCommands(device);

    vk::ImageMemoryBarrier barrier{};
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 6;  // All faces of the cubemap
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, { barrier });

        // Loop through all faces of the cubemap
        for (uint32_t face = 0; face < 6; face++) {
            VkImageBlit blit{};
            blit.srcOffsets[0] = { 0,0,0 };
            blit.srcOffsets[1] = { mipWidth,mipHeight,1 };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = face;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = { 0,0,0 };
            blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = face;
            blit.dstSubresource.layerCount = 1;

            commandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, { blit }, vk::Filter::eLinear);
        }

        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, nullptr, nullptr, { barrier });
        
        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    // Final barrier for the last mip level
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, { barrier });

    endSingleTimeCommands(device, commandBuffer);
}

void transitionImageLayoutForCubemap(std::shared_ptr<Device> device, Image& image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels) {
    vk::CommandBuffer commandBuffer = beginSingleTimeCommands(device);

    vk::ImageMemoryBarrier barrier{};
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;

    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspectFlags;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 6;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.srcAccessMask = {};
    barrier.dstAccessMask = {};

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    switch (image.layout)
    {
    case vk::ImageLayout::eUndefined:
        // Image layout is undefined (or does not matter)
        // Only valid as initial layout
        // No flags required, listed only for completeness
        barrier.srcAccessMask = (vk::AccessFlagBits)0;
        break;

    case vk::ImageLayout::ePreinitialized:
        // Image is preinitialized
        // Only valid as initial layout for linear images, preserves memory contents
        // Make sure host writes have been finished
        barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
        break;

    case vk::ImageLayout::eColorAttachmentOptimal:
        // Image is a color attachment
        // Make sure any writes to the color buffer have been finished
        barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        break;

    case vk::ImageLayout::eDepthAttachmentOptimal:
        // Image is a depth/stencil attachment
        // Make sure any writes to the depth/stencil buffer have been finished
        barrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        break;

    case vk::ImageLayout::eTransferSrcOptimal:
        // Image is a transfer source
        // Make sure any reads from the image have been finished
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        break;

    case vk::ImageLayout::eTransferDstOptimal:
        // Image is a transfer destination
        // Make sure any writes to the image have been finished
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        break;

    case vk::ImageLayout::eShaderReadOnlyOptimal:
        // Image is read by a shader
        // Make sure any shader reads from the image have been finished
        barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
        break;
    default:
        // Other source layouts aren't handled (yet)
        break;
    }

    // Target layouts (new)
    // Destination access mask controls the dependency for the new image layout
    switch (newLayout)
    {
    case vk::ImageLayout::eTransferDstOptimal:
        // Image will be used as a transfer destination
        // Make sure any writes to the image have been finished
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        break;

    case vk::ImageLayout::eTransferSrcOptimal:
        // Image will be used as a transfer source
        // Make sure any reads from the image have been finished
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
        break;

    case vk::ImageLayout::eColorAttachmentOptimal:
        // Image will be used as a color attachment
        // Make sure any writes to the color buffer have been finished
        barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        break;

    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
        // Image layout will be used as a depth/stencil attachment
        // Make sure any writes to depth/stencil buffer have been finished
        barrier.dstAccessMask = barrier.dstAccessMask | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        break;

    case vk::ImageLayout::eShaderReadOnlyOptimal:
        // Image will be read in a shader (sampler, input attachment)
        // Make sure any writes to the image have been finished
        if (barrier.srcAccessMask == (vk::AccessFlagBits)0)
        {
            barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite | vk::AccessFlagBits::eTransferWrite;
        }
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        break;
    default:
        // Other source layouts aren't handled (yet)
        break;
    }


    //TODO
    sourceStage = (vk::PipelineStageFlagBits)65536U;
    destinationStage = (vk::PipelineStageFlagBits)65536U;

    commandBuffer.pipelineBarrier(sourceStage, destinationStage, vk::DependencyFlags(), nullptr, nullptr, barrier);
    endSingleTimeCommands(device, commandBuffer);
}