/* signaturebind.cpp */
//#include <boost/bind.hpp>
#include <functional>
#include <iostream>
#include <string>
#include <tuple>

void identity(const std::string_view &name, int age, float height)
{
	std::cout << "Name   : " << name << '\n'
		  << "Age    : " << age << " years old\n"
		  << "Height : " << height << "inch\n";
}

int main(int argc, char * argv[])
{
	std::ignore = argc;
	std::ignore = argv;

	std::bind(&identity, "John", 25, 68.89f)();
	return 0;
}
