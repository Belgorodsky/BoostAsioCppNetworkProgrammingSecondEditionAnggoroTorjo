/* rangen_boost.cpp */
#include <boost/random/random_device.hpp>
#include <random>
#include <iostream>

int main()
{
	int guessNumber;
	std::cout << "Select number among 0 to 10: \n";
	std::cin >> guessNumber;
	if (guessNumber < 0 || 10 < guessNumber)
		return 1;
	boost::random::random_device rng; // std::random_device has bug for mingw 
	std::uniform_int_distribution<> ten(0, 10);
	int randomNumber = ten(rng);
	if (guessNumber == randomNumber)
		std::cout << "Congratulation, " << guessNumber << " is your lucky number.\n";
	else
		std::cout << "Sorry, I'm thinking about number " << randomNumber << '\n';

	return 0;
}
