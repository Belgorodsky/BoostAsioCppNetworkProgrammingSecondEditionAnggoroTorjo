/* multithreads.cpp */ 
#include <boost/asio.hpp>
#include <thread>
#include <array>
#include <algorithm>
#include <type_traits>
#include <memory>
#include <iostream>

boost::asio::io_context io_ctx;

void WorkerThread(int a)
{
	std::cout << a << ".\n";
	io_ctx.run();
	std::cout << "End.\n";
}

int main()
{
	auto worker = std::make_unique<boost::asio::io_service::work>(io_ctx);

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
				return std::decay_t<decltype(th)>(WorkerThread, n);
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
