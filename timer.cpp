/* timer.cpp */
#include <boost/asio.hpp>
#include <boost/current_function.hpp>
#include <thread>
#include <array>
#include <algorithm>
#include <type_traits>
#include <mutex>
#include <iostream>

std::mutex global_stream_lock;

void WorkerThread(boost::asio::io_context& ioctx, size_t counter)
{
	{
		std::lock_guard lck(global_stream_lock);
		std::cout << "Thread#" << std::this_thread::get_id() << " " << BOOST_CURRENT_FUNCTION << " " << counter << " Start.\n";
	}

	while (true)
	{
		try
		{
			boost::system::error_code ec;
			ioctx.run(ec);

			std::lock_guard lck(global_stream_lock);

			if (ec)
				std::cout << "Thread#" << std::this_thread::get_id() << " " << BOOST_CURRENT_FUNCTION << " Error Message: " << ec << '\n';
			break;
		}
		catch(std::exception &e)
		{
			std::lock_guard lck(global_stream_lock);
			std::cout << "Thread#" << std::this_thread::get_id() << " " << BOOST_CURRENT_FUNCTION << " Exception Message: " << e.what() << '\n';
		}
	}

	std::lock_guard lck(global_stream_lock);
	std::cout << "Thread#" << std::this_thread::get_id() << " " << BOOST_CURRENT_FUNCTION << " " << counter << " End.\n";
}

void TimerHandler(const boost::system::error_code &ec)
{
	std::lock_guard lck(global_stream_lock);
	if (ec)
	{
		std::cout << "Thread#" << std::this_thread::get_id() << " " << BOOST_CURRENT_FUNCTION << " Error Message: " << ec << '\n';
		return;
	}

	std::cout << "Thread#" << std::this_thread::get_id() << " " << BOOST_CURRENT_FUNCTION << " You see this line because you have waited for 10 seconds'\n";
	std::cout << "Thread#" << std::this_thread::get_id() << " " << BOOST_CURRENT_FUNCTION << " Now press ENTER to exit \n";
}

int main()
{
	boost::asio::io_context io_ctx;
	auto worker = boost::asio::make_work_guard(io_ctx);

	{
		std::lock_guard lck(global_stream_lock);
		std::cout << "Thread#" << std::this_thread::get_id() << " " << BOOST_CURRENT_FUNCTION << " The program will exit once all work has finished.\n";
	}

	std::array<std::thread, 5u> threads;
	{
		size_t i = 0;
		std::transform(
			threads.cbegin(),
			threads.cend(),
			threads.begin(),
			[fn=&WorkerThread,&io=io_ctx,&n=i](auto &&th)
			{
				++n;
				return std::decay_t<decltype(th)>(fn,std::ref(io),n);
			}
		);
	}

	boost::asio::deadline_timer timer(io_ctx);
	timer.expires_from_now(boost::posix_time::seconds(10));
	timer.async_wait(TimerHandler);

	std::cin.get();

	io_ctx.stop();

	for (auto &&th : threads)
		if (th.joinable())
			th.join();

	return 0;
}
