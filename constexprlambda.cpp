/* constexprlambda.cpp */
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

	[func=&identity, name="John", a=25, h=68.89f]() constexpr
	{
		func(name, a, h);
	}();
	return 0;
}
