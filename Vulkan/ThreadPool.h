#pragma once
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
struct DThread {
	std::unique_ptr<std::thread> worker_thread;
	vk::raii::CommandPool pool;
	vk::raii::CommandBuffer buffer;
};
//TODO, 추후에 Task 별로 상속받아 사용할 것.
class DThreadPool
{
public:
	size_t num_threads = std::thread::hardware_concurrency() - 1;

	DThreadPool(DContext& context) { init(context); }
	DThreadPool(DContext& context,size_t num_threads) : num_threads(num_threads){ init(context); }
	~DThreadPool();

	DThreadPool(const DThreadPool& dThreadPool) = delete;
	DThreadPool& operator=(const DThreadPool& dThreadPool) = delete;
	DThreadPool(DThreadPool&& dThreadPool) = default;
	DThreadPool& operator=(DThreadPool&& dThreadPool) = default;

	void begin(vk::CommandBufferBeginInfo beginInfo);
	void end();

	void reset() {
		for (auto& dthread : worker_threads) {
			dthread.buffer.reset();
		}
	}

	template <class F, class... Args>
	std::future<typename std::invoke_result<F,DThread&,Args...>::type> EnqueueJob(F&& f, Args&&... args);
private:
	void init(DContext& context);
	void WorkerThread(int idx);
	//default thread pool size is the number of hardware threads

	std::vector<DThread> worker_threads;
	std::queue<std::function<void(DThread&)>> jobs;
	std::condition_variable cv_job_q;
	std::mutex m_job_q;
	bool stop_all;
};

template<class F, class ...Args>
std::future<typename std::invoke_result<F,DThread&,Args...>::type> DThreadPool::EnqueueJob(F&& f, Args&& ...args)
{
	if (stop_all) throw std::runtime_error("ThreadPool 사용 중지됨");

	using return_type = typename std::invoke_result<F,DThread&,Args...>::type;
	auto job = std::make_shared<std::packaged_task<return_type(DThread&)>>(std::bind(std::forward<F>(f), std::placeholders::_1, std::forward<Args>(args)...));

	std::future<return_type> job_result_future = job->get_future();
	{
		std::lock_guard<std::mutex> lock(m_job_q);
		jobs.push([job](DThread& thread) {(*job)(thread); });

	}
	cv_job_q.notify_one();

	return job_result_future;
	
}