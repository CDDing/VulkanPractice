#include "pch.h"
#include "ThreadPool.h"

DThreadPool::~DThreadPool()
{
	stop_all = true;
	cv_job_q.notify_all();

	for (auto& t : worker_threads) {
		t.worker_thread->join();
	}
}

void DThreadPool::begin(vk::CommandBufferBeginInfo beginInfo)
{
	for (auto& dThread : worker_threads) {
		auto& commandBuffer = dThread.buffer;
		commandBuffer.reset({});
		commandBuffer.begin(beginInfo);
	}
}

void DThreadPool::end()
{
	for (auto& dThread : worker_threads) {
		auto& commandBuffer = dThread.buffer;
		commandBuffer.end();
	}
}


void DThreadPool::init(DContext& context)
{
	stop_all = false;
	for (size_t i = 0; i < num_threads; i++) {
		auto worker_thread = std::make_unique<std::thread>([this, i]() { this->WorkerThread(i); });
		
		//Command Pool
		auto indices = findQueueFamilies(context.physical, context.surface);
		vk::CommandPoolCreateInfo cmdPoolInfo{};
		cmdPoolInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
		cmdPoolInfo.setQueueFamilyIndex(indices.graphicsFamily.value());
		auto commandPool = vk::raii::CommandPool(context.logical, cmdPoolInfo);

		vk::CommandBufferAllocateInfo allocInfo{ commandPool,vk::CommandBufferLevel::eSecondary,1 };
		
		auto cmdBuf = (std::move(vk::raii::CommandBuffers(context.logical, allocInfo).front()));

		DThread d{ std::move(worker_thread),std::move(commandPool), std::move(cmdBuf)};
		worker_threads.push_back(std::move(d));
	}
}

void DThreadPool::WorkerThread(int idx)
{
	while (true) {
		std::unique_lock<std::mutex> lock(m_job_q);
		cv_job_q.wait(lock, [this]() {return !this->jobs.empty() || stop_all; });
		if (stop_all && this->jobs.empty()) return;

		std::function<void(DThread&)> job = std::move(jobs.front());
		jobs.pop();
		lock.unlock();

		job(worker_threads[idx]);
	}
}
