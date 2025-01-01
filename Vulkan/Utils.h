#pragma once
class Device;
uint32_t findMemoryType(Device& device, uint32_t typeFilter, VkMemoryPropertyFlags properties);
std::ostream& operator<<(std::ostream& os, const glm::vec3& vec);