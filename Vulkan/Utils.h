#pragma once
class Device;
class Buffer;
uint32_t findMemoryType(vk::PhysicalDevice& device, uint32_t typeFilter, vk::MemoryPropertyFlags properties);
std::ostream& operator<<(std::ostream& os, const glm::vec3& vec);
static void check_vk_result(VkResult err)
{
    if (err == VK_SUCCESS)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}
uint32_t SBTalignedSize(uint32_t value, uint32_t alignment);
vk::CommandBuffer beginSingleTimeCommands(std::shared_ptr<Device> device);
void endSingleTimeCommands(std::shared_ptr<Device> device, vk::CommandBuffer commandBuffer);
void copyBuffer(std::shared_ptr<Device> device, Buffer& srcBuffer, Buffer& dstBuffer, vk::DeviceSize size);