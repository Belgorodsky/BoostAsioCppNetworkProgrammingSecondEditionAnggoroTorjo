/* timer3.cpp */
#include <boost/asio.hpp>
#include <boost/current_function.hpp>
#include <thread>
#include <chrono>
#include <array>
#include <type_traits>
#include <mutex>
#include <iostream>
#include <memory>

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

void TimerHandler(const boost::system::error_code &ec, boost::asio::deadline_timer &tmr, boost::asio::io_context::strand &st)
{
	{
		std::lock_guard lck(global_stream_lock);
		if (ec)
		{
			std::cout << "Thread#" << std::this_thread::get_id() << " " << BOOST_CURRENT_FUNCTION << " Error Message: " << ec << '\n';
			return;
		}

		std::cout << "Thread#" << std::this_thread::get_id() << " " << BOOST_CURRENT_FUNCTION << " You see this every second'\n";
	}
	tmr.expires_from_now(boost::posix_time::seconds(1));
	tmr.async_wait(
		st.wrap(
			[fn=&TimerHandler,&t=tmr,&st](auto &&ec) mutable 
			{
				fn(ec,t,st);
			}
		)
	);
}

void Print(int number)
{
	std::cout << "Thread#" << std::this_thread::get_id() << " " << BOOST_CURRENT_FUNCTION << " Number: " << number << '\n';
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

int main()
{
	boost::asio::io_context io_ctx;
	auto worker = boost::asio::make_work_guard(io_ctx);
	boost::asio::io_context::strand strand(io_ctx);

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
				return std::decay_t<decltype(th)>(fn, std::ref(io), n);
			}
		);
	}
	
	std::this_thread::sleep_for(std::chrono::seconds(1));

	for (size_t i = 0; i !=5; )
		strand.post([fn=&Print,n=++i](){fn(n);});

	boost::asio::deadline_timer timer(io_ctx);
	timer.expires_from_now(boost::posix_time::seconds(1));
	timer.async_wait(
		strand.wrap(
			[fn=&TimerHandler,&t=timer,&st=strand](auto &&ec) mutable
			{
				fn(ec,t,st);
			}
		)
	);

	std::cin.get();

	io_ctx.stop();

	for (auto &&th : threads)
		if (th.joinable())
			th.join();

	return 0;
}
