/* rangen.cpp */
#include <cstdlib>
#include <iostream>
#include <ctime>

int main()
{
	int guessNumber;
	std::cout << "Select number among 0 to 10:\n";
	std::cin >> guessNumber;
	
	if (guessNumber < 0 || guessNumber > 10)
		return 1;
		
	std::srand(std::time(0));
	int randomNumber = std::rand() % (10 + 1);
	
	if (randomNumber == guessNumber)
		std::cout << "Congratulation , " << guessNumber << " is your lucky number.\n";
	else	
		std::cout << "Sorry, I'm thinking about number \n" << randomNumber << "\n";
		
	return 0;
	
}