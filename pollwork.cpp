/* pollwork.cpp */
#include <boost/asio.hpp>
#include <iostream>

int main()
{
	boost::asio::io_context io_ctx;
	auto worker = boost::asio::make_work_guard(io_ctx);

	for (size_t i = 0; i != 5; ++i)
	{
		io_ctx.poll();
		std::cout << "Line " << i << '\n';
	}

	return 0;
}
