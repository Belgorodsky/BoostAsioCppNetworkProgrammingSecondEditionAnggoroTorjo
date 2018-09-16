/* connectsync.cpp */
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/current_function.hpp>
#include <thread>
#include <array>
#include <algorithm>
#include <type_traits>
#include <mutex>
#include <iostream>

std::mutex global_stream_lock;

void WorkerThread(boost::asio::io_context &ioctx, size_t counter)
{
	{
		std::lock_guard lck(global_stream_lock);
		std::cout << "Thread#" << std::this_thread::get_id() 
			  << " " << BOOST_CURRENT_FUNCTION << " " 
			  << counter << " Start.\n";
	}

	while (true)
	{
		try
		{
			boost::system::error_code ec;
			ioctx.run(ec);
			if (ec)
			{
				std::lock_guard lck(global_stream_lock);
				std::cout << "Thread#" << std::this_thread::get_id()
				       	  << " " << BOOST_CURRENT_FUNCTION
					  << " Error Message: " << ec << '\n';
			}
			break;
		}
		catch (std::exception &e)
		{
			std::lock_guard lck(global_stream_lock);
			std::cout << "Thread#" << std::this_thread::get_id()
			       	  << " " << BOOST_CURRENT_FUNCTION
				  << " Exception Message: " << e.what() << '\n';
		}
	}
	
	std::lock_guard lck(global_stream_lock);
	std::cout << "Thread#" << std::this_thread::get_id()
	       	  << " " << BOOST_CURRENT_FUNCTION << " "
		  << counter << " End.\n";
}

int main()
{
	boost::asio::io_context io_ctx;
	auto worker = boost::asio::make_work_guard(io_ctx);
	{
		std::lock_guard lck(global_stream_lock);
		std::cout << "Thread#" << std::this_thread::get_id() << " "
		       	  << BOOST_CURRENT_FUNCTION << 
			  " Press ENTER to exit!\n";
	}

	std::array<std::thread, 2u> threads;
	{
		size_t i = 0;
		std::transform(
			threads.cbegin(),
			threads.cend(),
			threads.begin(),
			[&io=io_ctx,&n=i](auto &&th)
			{
				++n;
				return std::decay_t<decltype(th)>(&WorkerThread, std::ref(io), n);
			}
		);
	}

	boost::asio::ip::tcp::socket sckt(io_ctx);

	try
	{
		boost::asio::ip::tcp::resolver resolver(io_ctx);
		boost::asio::ip::tcp::resolver::query query("www.packtpub.com", "80");
		boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
		boost::asio::ip::tcp::endpoint endpoint = *iterator;
		{
			std::lock_guard lck(global_stream_lock);
			std::cout << "Thread#" << std::this_thread::get_id()
				  << " " << BOOST_CURRENT_FUNCTION
				  << " Connecting to: " << endpoint << '\n';
		}

		sckt.connect(endpoint);
		{
			std::lock_guard lck(global_stream_lock);
			std::cout << "Thread#" << std::this_thread::get_id()
				  << " " << BOOST_CURRENT_FUNCTION
				  << " Connected!\n";
		}
	}
	catch (std::exception &e)
	{
		std::lock_guard lck(global_stream_lock);
		std::cout << "Thread#" << std::this_thread::get_id()
			  << " " << BOOST_CURRENT_FUNCTION
			  << " Exception Message: " << e.what() << '\n';
	}

	std::cin.get();

	boost::system::error_code ec;
	sckt.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
	sckt.close(ec);

	io_ctx.stop();

	for (auto &&th : threads)
		if (th.joinable())
			th.join();

	return 0;
}
