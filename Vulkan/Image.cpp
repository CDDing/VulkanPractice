#include "pch.h"
#include "Image.h"

void DImage::setImageLayout(vk::raii::CommandBuffer& commandBuffer, vk::ImageLayout newLayout) {
    vk::ImageAspectFlags aspectMask;
    if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
    {
        aspectMask = vk::ImageAspectFlagBits::eDepth;
        if (format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint)
        {
            aspectMask |= vk::ImageAspectFlagBits::eStencil;
        }
    }
    else
    {
        aspectMask = vk::ImageAspectFlagBits::eColor;
    }

    vk::ImageMemoryBarrier imageMemoryBarrier{};
    imageMemoryBarrier.oldLayout = layout;
    imageMemoryBarrier.newLayout = newLayout;

    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange.aspectMask = aspectMask;
    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.layerCount = 1;
    imageMemoryBarrier.subresourceRange.levelCount = mipLevels;
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
            imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eHostWrite | vk::AccessFlagBits::eTransferWrite;
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

    commandBuffer.pipelineBarrier(sourceStage, destinationStage, vk::DependencyFlags(), nullptr, nullptr, imageMemoryBarrier);
    layout = newLayout;
}

void DImage::copyFromBuffer(vk::raii::CommandBuffer& commandBuffer, vk::raii::Buffer& buffer)
{

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
        extent.width,extent.height,1 };

    commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, { region });
}

void DImage::generateMipmaps(vk::raii::CommandBuffer& commandBuffer)
{
    if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        throw std::runtime_error("texture image format does not linear blitting!");
    }

    vk::ImageMemoryBarrier barrier{};
    barrier.image = *image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;


    int32_t mipWidth = extent.width;
    int32_t mipHeight = extent.height;

    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, { barrier });

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

        commandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, { blit }, vk::Filter::eLinear);

        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, { barrier });

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1)mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

	commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, { barrier });
}

void CubemapImage::setImageLayout(vk::raii::CommandBuffer& commandBuffer, vk::ImageLayout newLayout)
{

    vk::ImageMemoryBarrier barrier{};
    barrier.oldLayout = layout;
    barrier.newLayout = newLayout;

    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 6;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.srcAccessMask = {};
    barrier.dstAccessMask = {};

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    switch (layout)
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

    commandBuffer.pipelineBarrier(sourceStage, destinationStage, vk::DependencyFlags(), {}, {}, barrier);
}

void CubemapImage::copyFromBuffer(vk::raii::CommandBuffer& commandBuffer, vk::raii::Buffer& buffer)
{

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.layerCount = 1;  // 한 레이어씩 복사

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { extent.width, extent.height, 1 };

    // 6개의 레이어에 대해 반복
    for (uint32_t layer = 0; layer < 6; layer++) {
        region.imageSubresource.baseArrayLayer = layer;  // 각 레이어에 대해 baseArrayLayer 설정
        region.bufferOffset = layer * layerSize;

        commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, { region });
    }
}

void CubemapImage::generateMipmaps(vk::raii::CommandBuffer& commandBuffer)
{
    if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    
    vk::ImageMemoryBarrier barrier{};
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 6;  // All faces of the cubemap
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = extent.width;
    int32_t mipHeight = extent.height;

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

		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, { barrier });
        
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

}
void generateMipmapsForCubemap(std::shared_ptr<Device> device, vk::Image image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
    
}
