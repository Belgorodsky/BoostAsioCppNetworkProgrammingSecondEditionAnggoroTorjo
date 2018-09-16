/* serverasync.cpp */
#include <boost/asio.hpp>
#include <boost/current_function.hpp>
#include <thread>
#include <mutex>
#include <array>
#include <iostream>
#include <string>

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
					  << " " << BOOST_CURRENT_FUNCTION << " Error message: "
					  << ec << ".\n";
			}
			break;
		}
		catch (std::exception &e)
		{
			std::lock_guard lck(global_stream_lock);
			std::cout << "Thread#" << std::this_thread::get_id()
				  << " " << BOOST_CURRENT_FUNCTION << " Exception message: "
				  << e.what() << ".\n";
		}
	}

	std::lock_guard lck(global_stream_lock);
	std::cout << "Thread#" << std::this_thread::get_id()
		  << " " << BOOST_CURRENT_FUNCTION << " "
		  << counter << " End.\n";
}

void OnAccept(const boost::system::error_code &ec)
{
	std::lock_guard lck(global_stream_lock);
	if (ec)
	{
		std::cout << "Thread#" << std::this_thread::get_id()
			  << " " << BOOST_CURRENT_FUNCTION << " Error message: "
			  << ec << ".\n";
		return;
	}

	std::cout << "Thread#" << std::this_thread::get_id()
		<< " " << BOOST_CURRENT_FUNCTION << " Accepted!\n";
}

int main()
{
	boost::asio::io_context io_ctx;
	auto worker = boost::asio::make_work_guard(io_ctx);

	{
		std::lock_guard lck(global_stream_lock);
		std::cout << "Thread#" << std::this_thread::get_id()
		       	  << " " << BOOST_CURRENT_FUNCTION
			  << " Press ENTER to exit!\n";
	}

	std::array threads {
	       	std::thread(WorkerThread, std::ref(io_ctx), 1u),
	       	std::thread(WorkerThread, std::ref(io_ctx), 2u),
	};

	boost::asio::ip::tcp::acceptor acceptor(io_ctx);

	boost::asio::ip::tcp::socket sckt(io_ctx);

	try
	{
		boost::asio::ip::tcp::resolver resolver(io_ctx);
		boost::asio::ip::tcp::resolver::query query("127.0.0.1", "4444");
		boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
		acceptor.open(endpoint.protocol());
		acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(false));
		acceptor.bind(endpoint);
		acceptor.listen(boost::asio::socket_base::max_connections);
		{
			std::lock_guard lck(global_stream_lock);
			std::cout << "Thread#" << std::this_thread::get_id()
				  << " " << BOOST_CURRENT_FUNCTION
				  << " Listening on: " << endpoint << '\n';
		}
		acceptor.async_accept(sckt, &OnAccept);
	}
	catch (std::exception &e)
	{
		std::lock_guard lck(global_stream_lock);
		std::cout << "Thread#" << std::this_thread::get_id()
		       	  << " " << BOOST_CURRENT_FUNCTION
			  << " Exception message: " << e.what() << '\n';
	}
	std::cin.get();

	boost::system::error_code ec;
	acceptor.close(ec);

	sckt.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
	sckt.close(ec);

	io_ctx.stop();

	for (auto &&th: threads)
		if (th.joinable())
			th.join();

	return 0;
}
