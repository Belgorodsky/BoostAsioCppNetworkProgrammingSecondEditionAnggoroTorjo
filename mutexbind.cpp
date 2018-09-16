/* mutexbind.cpp */
#include <boost/asio.hpp>
#include <thread>
#include <array>
#include <algorithm>
#include <type_traits>
#include <iostream>
#include <mutex>

std::mutex global_stream_lock;

void WorkerThread(boost::asio::io_context &ioctx, int counter)
{
	{
		std::lock_guard lck(global_stream_lock);
		std::cout << counter << ".\n";
	}

	ioctx.run();

	{
		std::lock_guard lck(global_stream_lock);
		std::cout << "End.\n";
	}
}

int main()
{
	boost::asio::io_context io_ctx;
	auto worker = boost::asio::make_work_guard(io_ctx);

	std::cout << "Press ENTER key to exit!\n";

	std::array<std::thread, 5u> threads;
	{
		size_t i = 0;
		std::transform(
			threads.begin(),
			threads.end(),
			threads.begin(),
			[&n=i,&io=io_ctx](auto &&th)
			{
				++n;
				return std::decay_t<decltype(th)>(&WorkerThread, std::ref(io), n);
			}
		);
	}

	std::cin.get();

	io_ctx.stop();

	for (auto &&th : threads)
		if (th.joinable())
			th.join();

	return 0;
}
