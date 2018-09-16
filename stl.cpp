/* stl.cpp */
#include <vector>
#include <iostream>
#include <algorithm>
#include <iterator>

int main()
{
	int temp; 
	std::vector<int> collection;
	std::cout << "Please input the collection of integer numbers, intput 0 to STOP\n";
	while (std::cin >> temp)
	{
		if (temp == 0) break;
		collection.push_back(temp);
	}
	std::sort(collection.begin(), collection.end());
	std::cout << "The sort collection of your integer numbers:\n";
	std::copy(collection.cbegin(), collection.cend(), std::ostream_iterator<int>(std::cout, "\n"));

	return 0;
}
