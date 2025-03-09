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


void DThreadPool::init(DContext& context)
{
	stop_all = false;
	for (size_t i = 0; i < num_threads; i++) {
		auto worker_thread = std::make_unique<std::thread>([this]() { this->WorkerThread(); });
		


		DThread d{ std::move(worker_thread),vk::raii::CommandPool(nullptr) };
		worker_threads.push_back(std::move(d));
	}
}

void DThreadPool::WorkerThread()
{
	while (true) {
		std::unique_lock<std::mutex> lock(m_job_q);
		cv_job_q.wait(lock, [this]() {return !this->jobs.empty() || stop_all; });
		if (stop_all && this->jobs.empty()) return;

		std::function<void()> job = std::move(jobs.front());
		jobs.pop();
		lock.unlock();

		job();
	}
}
