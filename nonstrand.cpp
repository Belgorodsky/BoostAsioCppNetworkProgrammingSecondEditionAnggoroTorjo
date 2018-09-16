/* nonstrand.cpp */
#include <boost/asio.hpp>
#include <thread>
#include <iostream>
#include <mutex>
#include <chrono>
#include <array>

std::mutex global_stream_lock;

void WorkerThread(
    boost::asio::io_context &ioctx,
    size_t counter
)
{
    {
        // c++17 code, if not compile write std::lock_guard<std::mutex> lck(global_stream_lock); 
        std::lock_guard lck(global_stream_lock);
        std::cout << "thread#" << std::this_thread::get_id() << " Thread " << counter << " Start.\n";
    }

    ioctx.run();

    {
        std::lock_guard lck(global_stream_lock);
        std::cout << "thread#" << std::this_thread::get_id() << " Thread " << counter << " End.\n";
    }
}

void Print(int number)
{
	std::lock_guard lck(global_stream_lock);
	std::cout << "thread#" << std::this_thread::get_id() << " Number: " << number << '\n';
}

int main()
{
    boost::asio::io_context io_ctx;
    auto worker = boost::asio::make_work_guard(io_ctx);
    {
        std::lock_guard lck(global_stream_lock);
        std::cout << "thread#" << std::this_thread::get_id() << " " << "The program will exit once all work has finished.\n";
    }

    std::array<std::thread, 5u> threads;
    for (size_t i = 1; i !=6; ++i)
        threads[i-1] = std::move(std::thread(WorkerThread, std::ref(io_ctx), i));

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    for (size_t i = 1; i !=6; ++i)
        io_ctx.post([func=&Print, n=i]() { func(n); });

    worker.reset();

    for(auto &&th : threads) if (th.joinable()) th.join();

    return 0;
}

