#include "pch.h"
#include "Queue.h"
QueueFamilyIndices Queue::_indices = {};

Queue::Queue()
{
}

Queue::Queue(Device* device)
{
    //_queues.resize(QueueType::END);
    for (int i = 0; i < QueueType::END; i++) {
        VkQueue q;
        _queues.push_back(q);
    }
    vkGetDeviceQueue(device->Get(), _indices.graphicsFamily.value(), 0, &_queues[QueueType::GRAPHICS]);
    vkGetDeviceQueue(device->Get(), _indices.presentFamily.value(), 0, &_queues[QueueType::PRESENT]);
}

VkQueue& Queue::Get(QueueType type)
{
    return _queues[type];
}

