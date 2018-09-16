/* errorcode.cpp */
#include <boost/asio.hpp>
#include <boost/current_function.hpp>
#include <thread>
#include <array>
#include <mutex>
#include <iostream>
#include <type_traits>

std::mutex global_stream_lock;

void WorkerThread(boost::asio::io_context &ioctx, size_t counter)
{
	{
		// c++17 code; if not compile use the std::lock_guard<std::mutex> lck(global_stream_lock) 
		std::lock_guard lck(global_stream_lock);
		std::cout << "Thread#" << std::this_thread::get_id() << " " << BOOST_CURRENT_FUNCTION << " " << counter << " Start.\n";
	}

	boost::system::error_code ec;
	ioctx.run(ec);

	std::lock_guard lck(global_stream_lock);

	if (ec)
	{
		std::cout << "Thread#" << std::this_thread::get_id() << " " << BOOST_CURRENT_FUNCTION << " Message: " << ec << '\n';
	}

	std::cout << "Thread#" << std::this_thread::get_id() << " " << BOOST_CURRENT_FUNCTION << " " << counter << " End.\n";
}

void ThrowAnException(boost::asio::io_context &ioctx)
{
	{
		std::lock_guard lck(global_stream_lock);
		std::cout << "Thread#" << std::this_thread::get_id() << " " << BOOST_CURRENT_FUNCTION << '\n';
	}

	ioctx.post([fn=&ThrowAnException,&io=ioctx]{fn(io);});

	throw(std::runtime_error("The Exception !!!"));
}

int main()
{
	boost::asio::io_context io_ctx;
	auto worker = boost::asio::make_work_guard(io_ctx);

	{
		std::lock_guard lck(global_stream_lock);
		std::cout << "Thread#" << std::this_thread::get_id() << " " << BOOST_CURRENT_FUNCTION << " The program will exit once all work has finished.\n";
	}
	
	std::array<std::thread, 5> threads;
	{
		size_t i = 0;
		std::transform(
			threads.begin(),
			threads.end(),
			threads.begin(),
			[&n=i, &io=io_ctx](auto &&th)
			{
				++n;
				return std::decay_t<decltype(th)>(&WorkerThread, std::ref(io), n);
			}
		);
	}

	io_ctx.post([fn=&ThrowAnException,&io=io_ctx]{fn(io);});

	for (auto &&th : threads)	
		if (th.joinable())
			th.join();

	return 0;
}
