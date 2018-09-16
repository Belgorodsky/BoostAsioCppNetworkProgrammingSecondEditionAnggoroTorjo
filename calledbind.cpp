/* calledbind.cpp */
#include <boost/bind.hpp>
#include <iostream>

void func()
{
	std::cout << "Binding Function\n";
}

int main()
{
	boost::bind(&func)();
	return 0;
}
