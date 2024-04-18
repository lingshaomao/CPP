#include <iostream>

#include "ThreadPool.h"

using namespace std;



long long compute_square(long long n) {
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	return n * n;
}

void compute_square_and_print(long long n) {
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	std::cout << "\n--Computed square of " << n << ": " << n * n << std::endl;
}


int main() {
	ThreadPool thread_pool(4);

	std::vector<std::pair<long long, std::shared_future<void>>> tasks = {
		{10, {}}, {20, {}}, {30, {}}, {40, {}}, {50, {}}, {60, {}}, {70, {}}, {80, {}}, {90, {}}, {100, {}}
	};

	// wait result
	for (auto& [n, future] : tasks) {
		future = thread_pool.enqueue(compute_square_and_print, n).share();
	}
	for (auto& [n, future] : tasks) {
		future.get();
	}

	// don't wait result
	for (int i = 1; i < 10; ++i)
	{
		thread_pool.enqueue(compute_square_and_print, i * 10);
	}
	this_thread::sleep_for(chrono::seconds(60));

	return 0;
}
