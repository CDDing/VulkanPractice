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
class DThreadPool
{
public:
	size_t num_threads = std::thread::hardware_concurrency() - 1;

	DThreadPool(DContext& context) { init(context); }
	DThreadPool(DContext& context,size_t num_threads) : num_threads(num_threads){ init(context); }
	~DThreadPool();
	template <class F, class... Args>
	std::future<typename std::invoke_result<F,Args...>::type> EnqueueJob(F&& f, Args&&... args);
private:
	struct DThread {
		std::unique_ptr<std::thread> worker_thread;
		vk::raii::CommandPool pool;
	};
	void init(DContext& context);
	void WorkerThread();
	//default thread pool size is the number of hardware threads

	std::vector<DThread> worker_threads;
	std::queue<std::function<void()>> jobs;
	std::condition_variable cv_job_q;
	std::mutex m_job_q;
	bool stop_all;
};

template<class F, class ...Args>
std::future<typename std::invoke_result<F,Args...>::type> DThreadPool::EnqueueJob(F&& f, Args&& ...args)
{
	if (stop_all) throw std::runtime_error("ThreadPool »ç¿ë ÁßÁöµÊ");

	using return_type = typename std::invoke_result<F,Args...>::type;
	auto job = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));

	std::future<return_type> job_result_future = job->get_future();
	{
		std::lock_guard<std::mutex> lock(m_job_q);
		jobs.push([job]() {(*job)(); });

	}
	cv_job_q.notify_one();

	return job_result_future;
	
}