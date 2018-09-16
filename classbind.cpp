/* classbind.cpp */
#include <boost/bind.hpp>
#include <iostream>
#include <string>

class TheClass
{
public:
	void identity(const std::string_view &name, int age, float height)
	{
		std::cout << "Name   : " << name << '\n'
			  << "Age    : " << age << " years old\n"
			  << "Height : " << height << "inch\n";
	}
};

int main()
{
	TheClass cls;
	boost::bind(&TheClass::identity, &cls, "John", 25 ,68.89f)();
	return 0;
}
