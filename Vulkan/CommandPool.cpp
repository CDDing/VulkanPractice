#include "pch.h"
#include "CommandPool.h"
vk::raii::CommandPool CommandPool::TransientPool = vk::raii::CommandPool(nullptr);