/* unblocked.cpp */
#include <boost/asio.hpp>
#include <iostream>

int main()
{
	boost::asio::io_service io_svc;

	io_svc.run();

	std::cout << "We will see this line in console windows." << std::endl;

	return 0;
}
