/* hash.cpp */
#include <iostream>
#include <functional>
#include <string>

int main()
{
	std::string plainText;
	std::cout << "Input string and hit Enter if ready: \n";
	std::cin >> plainText;
	std::hash<std::string> hashFunc;
	size_t hashText = hashFunc(plainText);
	std::cout << "Hashing: " << hashText << '\n';
	return 0;
}
