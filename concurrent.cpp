/* concurrent.cpp */
#include <thread>
#include <array>
#include <chrono>
#include <algorithm>
#include <iostream>

void Print1()
{
	for (size_t i = 0; i != 5; ++i)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds{500});
		std::cout << "[Print1] Line: " << i << '\n';
	}
}

void Print2()
{
	for (size_t i = 0; i != 5; ++i)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds{500});
		std::cout << "[Print2] Line: " << i << '\n';
	}
}

int main()
{
	std::array threads{ std::thread(Print1), std::thread(Print2) };
	for (auto &&th : threads)
		if (th.joinable())
			th.join();
	return 0;

}
