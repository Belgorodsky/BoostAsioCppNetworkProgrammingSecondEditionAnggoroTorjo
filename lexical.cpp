/* lexical.cpp */
#include <boost/lexical_cast.hpp>
#include <string>
#include <iostream>

int main()
{
	try
	{
		std::string str;
		std::cout << "Please input first number: \n";
		std::cin >> str;
		int n1 = boost::lexical_cast<int>(str);
		std::cout << "Please input second number: \n";
		std::cin >> str;
		int n2 = boost::lexical_cast<int>(str);
		std::cout << "The sum of the two numbers is " << n1 + n2 << '\n';
	}
	catch (const boost::bad_lexical_cast &e)
	{
		std::cerr << e.what() << '\n';
		return 1;
	}

	return 0;
}
