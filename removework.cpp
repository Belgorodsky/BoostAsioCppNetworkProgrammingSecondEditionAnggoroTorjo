/* removework.cpp */
#include <boost/asio.hpp>
#include <memory>
#include <iostream>

int main()
{
	boost::asio::io_context io_ctx;
	auto worker = boost::asio::make_work_guard(io_ctx);

	worker.reset();

	io_ctx.run();

	std::cout << "We will not see this line in console window :(\n";

	return 0;
}
