#pragma once
struct DImage {
	DImage(std::nullptr_t) : image(nullptr), view(nullptr), memory(nullptr) {}
	DImage() : image(nullptr), view(nullptr), memory(nullptr) {}
    //For Child
    DImage(DContext& context,
        uint32_t mipLevels,
        vk::Format format,
        vk::Extent2D extent,
        vk::ImageLayout layout,
        vk::MemoryPropertyFlags memoryProperties) :
        mipLevels(mipLevels), format(format), extent(extent), layout(layout), formatProperties(context.physical.getFormatProperties(format)),
        image(nullptr), view(nullptr), memory(nullptr) {}

    DImage(DContext& context,
        uint32_t mipLevels,
        vk::Format format,
        vk::Extent2D extent,
        vk::ImageTiling tiling,
        vk::ImageUsageFlags usage,
        vk::ImageLayout layout,
        vk::MemoryPropertyFlags memoryProperties,
        vk::ImageAspectFlags aspectMask) : 
		//Initialize Field
		format(format), layout(layout), extent(extent), mipLevels(mipLevels), formatProperties(context.physical.getFormatProperties(format)),
        image(nullptr),view(nullptr), memory(nullptr){
        
        //Create Image
        vk::ImageCreateInfo imageInfo{};
        imageInfo.setArrayLayers(1);
        imageInfo.setExtent({ extent.width, extent.height, 1 });
		imageInfo.setFormat(format);
		imageInfo.setImageType(vk::ImageType::e2D);
		imageInfo.setMipLevels(mipLevels);
		imageInfo.setSamples(vk::SampleCountFlagBits::e1);
		imageInfo.setTiling(tiling);
		imageInfo.setUsage(usage);
		imageInfo.setInitialLayout(layout);
        imageInfo.setSharingMode(vk::SharingMode::eExclusive);
        image = vk::raii::Image(context.logical, imageInfo);

        auto memRequirements = image.getMemoryRequirements();

		vk::MemoryAllocateInfo allocInfo{};
		allocInfo.setAllocationSize(memRequirements.size);
		allocInfo.setMemoryTypeIndex(findMemoryType(context.physical, memRequirements.memoryTypeBits, memoryProperties));
		
        memory = vk::raii::DeviceMemory(context.logical, allocInfo);

        image.bindMemory(memory, 0);

		//Create Image View
        vk::ImageViewCreateInfo imageViewInfo{};
        imageViewInfo.setImage(image);
        imageViewInfo.setFormat(format);
		imageViewInfo.setViewType(vk::ImageViewType::e2D);
		imageViewInfo.setSubresourceRange({ aspectMask, 0, mipLevels, 0, 1});
		view = vk::raii::ImageView(context.logical, imageViewInfo);
    }

    virtual void setImageLayout(vk::raii::CommandBuffer& commandBuffer, vk::ImageLayout newLayout);
	virtual void copyFromBuffer(vk::raii::CommandBuffer& commandBuffer, vk::raii::Buffer& buffer);
    virtual void generateMipmaps(vk::raii::CommandBuffer& commandBuffer);


    vk::raii::DeviceMemory memory{ nullptr };
    vk::raii::Image image{ nullptr };
    vk::raii::ImageView view{ nullptr };

    
    vk::Format format;
	vk::FormatProperties formatProperties;
    vk::ImageLayout layout;
    vk::Extent2D extent;
    uint32_t mipLevels;
};

struct CubemapImage : public DImage {
	CubemapImage(std::nullptr_t) : DImage(nullptr) {}
	CubemapImage(DContext& context,
        vk::DeviceSize layerSize,
		uint32_t mipLevels,
		vk::Format format,
		vk::Extent2D extent,
		vk::ImageTiling tiling,
		vk::ImageUsageFlags usage,
		vk::ImageLayout layout,
		vk::MemoryPropertyFlags memoryProperties,
		vk::ImageAspectFlags aspectMask) : DImage(context, mipLevels, format, extent, tiling, usage, layout, memoryProperties, aspectMask),
		layerSize(layerSize)
    {

        //Create Image
        vk::ImageCreateInfo imageInfo{};
        imageInfo.setArrayLayers(6);
        imageInfo.setExtent({ extent.width, extent.height, 1 });
        imageInfo.setFormat(format);
        imageInfo.setImageType(vk::ImageType::e2D);
        imageInfo.setMipLevels(mipLevels);
        imageInfo.setSamples(vk::SampleCountFlagBits::e1);
        imageInfo.setTiling(tiling);
        imageInfo.setUsage(usage);
        imageInfo.setInitialLayout(layout);
        imageInfo.setSharingMode(vk::SharingMode::eExclusive);
        imageInfo.setFlags(vk::ImageCreateFlagBits::eCubeCompatible);
        image = vk::raii::Image(context.logical, imageInfo);

        auto memRequirements = image.getMemoryRequirements();

        vk::MemoryAllocateInfo allocInfo{};
        allocInfo.setAllocationSize(memRequirements.size);
        allocInfo.setMemoryTypeIndex(findMemoryType(context.physical, memRequirements.memoryTypeBits, memoryProperties));

        memory = vk::raii::DeviceMemory(context.logical, allocInfo);

        image.bindMemory(memory, 0);

        //Create Image View
        vk::ImageViewCreateInfo imageViewInfo{};
        imageViewInfo.setImage(image);
        imageViewInfo.setFormat(format);
        imageViewInfo.setViewType(vk::ImageViewType::eCube);
        imageViewInfo.setSubresourceRange({ aspectMask, 0, mipLevels, 0, 6 });
        view = vk::raii::ImageView(context.logical, imageViewInfo);
	}

	void setImageLayout(vk::raii::CommandBuffer& commandBuffer, vk::ImageLayout newLayout) override;
	void copyFromBuffer(vk::raii::CommandBuffer& commandBuffer, vk::raii::Buffer& buffer) override;
	void generateMipmaps(vk::raii::CommandBuffer& commandBuffer) override;

    vk::DeviceSize layerSize;
};
