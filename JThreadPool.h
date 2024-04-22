#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread> // ʹ�� std::jthread �滻 std::thread
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <type_traits>

class JThreadPool {
public:
	JThreadPool(size_t);
	template<class F, class... Args>
	auto enqueue(F&& f, Args&&... args)
		-> std::future<typename std::invoke_result_t<F, Args...>>;
	~JThreadPool();

private:
	// ���ֶ��̵߳ĸ����Ա��ڱ�Ҫʱ��ֹ����
	std::vector<std::jthread> workers;
	// �������
	std::queue<std::function<void()>> tasks;

	// ͬ��
	std::mutex queue_mutex;
	std::condition_variable condition;
	bool stop;
	std::stop_source stop_source; // ���������߳���ֹ
};

// ���캯������ָ�������Ĺ����߳�
inline JThreadPool::JThreadPool(size_t threads)
	: stop(false), stop_source{}
{
	for (size_t i = 0; i < threads; ++i)
	{
		workers.emplace_back([this, stop_token = stop_source.get_token()] {
			for (;;)
			{
				std::function<void()> task;

				{
					std::unique_lock<std::mutex> lock(queue_mutex);
					condition.wait(lock, [this, stop_token] {
						return stop || stop_token.stop_requested() || !tasks.empty();
					});
					if (stop || stop_token.stop_requested() && tasks.empty())
						return;
					task = std::move(tasks.front());
					tasks.pop();
				}

				task();
			}
		});
	}
}

// ����¹�����̳߳�
template<class F, class... Args>
auto JThreadPool::enqueue(F&& f, Args&&... args)
-> std::future<typename std::invoke_result_t<F, Args...>>
{
	using return_type = typename std::invoke_result_t<F, Args...>;

	auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
	std::future<return_type> res = task->get_future();

	{
		std::unique_lock<std::mutex> lock(queue_mutex);

		if (stop)
			throw std::runtime_error("enqueue on stopped JThreadPool");

		tasks.emplace([task]() { (*task)(); });
	}
	condition.notify_one();
	return res;
}

// ����������ֹ�����߳�
inline JThreadPool::~JThreadPool()
{
	{
		std::unique_lock<std::mutex> lock(queue_mutex);
		stop = true;
	}
	stop_source.request_stop(); // �����߳���ֹ
	condition.notify_all();
}

#endif