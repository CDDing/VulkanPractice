#pragma once
class DContext;
struct DBuffer;
uint32_t findMemoryType(vk::raii::PhysicalDevice& device, uint32_t typeFilter, vk::MemoryPropertyFlags properties);
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
vk::raii::CommandBuffer beginSingleTimeCommands(DContext& context);
void endSingleTimeCommands(DContext& context, vk::raii::CommandBuffer& commandBuffer);
void copyBuffer(DContext& context, DBuffer& srcBuffer, DBuffer& dstBuffer, vk::DeviceSize size);