#pragma once
class Device;
uint32_t findMemoryType(Device& device, uint32_t typeFilter, VkMemoryPropertyFlags properties);
std::ostream& operator<<(std::ostream& os, const glm::vec3& vec);
static void check_vk_result(VkResult err)
{
    if (err == VK_SUCCESS)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}
