/* strandwrap.cpp */
#include <boost/asio.hpp>
#include <thread>
#include <iostream>
#include <mutex>
#include <chrono>
#include <array>
#include <algorithm>

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
    std::cout << "thread#" << std::this_thread::get_id() << " Number: " << number << '\n';
}

int main()
{
    boost::asio::io_context io_ctx;
    auto worker = boost::asio::make_work_guard(io_ctx);
    boost::asio::io_context::strand strand(io_ctx);
    {
        std::lock_guard lck(global_stream_lock);
        std::cout << "thread#" << std::this_thread::get_id() << " " << "The program will exit once all work has finished.\n";
    }

    std::array<std::thread, 5u> threads;
    {
	    size_t i = 0;
	    std::transform(
		threads.begin(), 
		threads.end(),
		threads.begin(), 
		[&io=io_ctx, &n=i](auto&& th) { ++n; return std::thread(WorkerThread, std::ref(io), n); }
	    );
    }

    for (size_t i = 0; i !=6; )
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        io_ctx.post(strand.wrap([func=&Print, n=++i]() { func(n); }));
        io_ctx.post(strand.wrap([func=&Print, n=++i]() { func(n); }));
    }

    worker.reset();

    for(auto &&th : threads) if (th.joinable()) th.join();

    return 0;
}
