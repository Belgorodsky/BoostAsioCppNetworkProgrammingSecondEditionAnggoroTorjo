/* blocked.cpp */
#include <boost/asio.hpp>
#include <iostream>

int main()
{
	boost::asio::io_context io_svc;
	auto worker = boost::asio::make_work_guard(io_svc);

	io_svc.run();

	std::cout << "We will see this line in console windows." << std::endl;

	return 0;
}
