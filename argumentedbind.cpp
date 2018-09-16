/* argumentedbind.cpp */
#include <boost/bind.hpp>
#include <iostream>

void cubevolume(float f)
{
	std::cout << "Volume of the cube is " << f * f * f << '\n';
}

int main()
{
	boost::bind(&cubevolume, _1)(4.23f);
	return 0;
}
