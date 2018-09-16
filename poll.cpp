/* poll.cpp */
#include <boost/asio.hpp>
#include <iostream>

int main()
{
	boost::asio::io_service io_svc;

	for (size_t i = 0; i != 5; ++i)
	{
		io_svc.poll();
		std::cout << "Line " << i << '\n';
	}

	return 0;
}
