#pragma once
struct DBuffer {
	DBuffer(std::nullptr_t) : buffer(nullptr), memory(nullptr) {}
	DBuffer(Device& device,
		vk::DeviceSize size,
		vk::BufferUsageFlags usage,
		vk::MemoryPropertyFlags propertyFlags) 
		:
		size(size), usage(usage), propertyFlags(propertyFlags),
		buffer(nullptr), memory(nullptr) {

		//Create Buffer
		vk::BufferCreateInfo bufferInfo{};
		bufferInfo.setFlags({});
		bufferInfo.setSize(size);
		bufferInfo.setUsage(usage);
		bufferInfo.setSharingMode(vk::SharingMode::eExclusive);
		buffer = vk::raii::Buffer(device.logical, bufferInfo);


		vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
		vk::MemoryAllocateInfo allocInfo{};
		allocInfo.setAllocationSize(memRequirements.size);
		allocInfo.setMemoryTypeIndex(findMemoryType(device.physical, memRequirements.memoryTypeBits, propertyFlags));
			vk::MemoryAllocateFlagsInfo allocFlagsInfo{};
			if (usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) {
				allocFlagsInfo.flags = vk::MemoryAllocateFlagBits::eDeviceAddress;
				allocInfo.pNext = &allocFlagsInfo;
			}
		memory = vk::raii::DeviceMemory(device.logical, allocInfo);

		buffer.bindMemory(memory, 0);

	}
	DBuffer(DBuffer&& other) noexcept = default;
	DBuffer& operator=(DBuffer&& other) noexcept = default;
	void unmap(Device& device);
	void map(Device& device, vk::DeviceSize size = vk::WholeSize, vk::DeviceSize offset = 0);
	void flush(Device& device, vk::DeviceSize size = vk::WholeSize, vk::DeviceSize offset = 0);
	void fillBuffer(Device& device, void* data, vk::DeviceSize size);
	vk::DescriptorBufferInfo GetBufferInfo();





	vk::DeviceSize size;
	vk::BufferUsageFlags usage;
	vk::MemoryPropertyFlags propertyFlags;
	void* mapped = nullptr;
	vk::raii::DeviceMemory memory{ nullptr };
	vk::raii::Buffer buffer{ nullptr };
};
