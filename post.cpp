/* post.cpp */
#include <boost/asio.hpp>
#include <thread>
#include <array>
#include <iostream>
#include <mutex>
#include <chrono>

std::mutex global_stream_lock;

void WorkerThread(
    boost::asio::io_context &ioctx,
    int counter
)
{
    {
        // c++17 code, if not compile write std::lock_guard<std::mutex> lck(global_stream_lock); 
        std::lock_guard lck(global_stream_lock);
        std::cout << "thread#" << std::this_thread::get_id() << " " << counter << ".\n";
    }

    ioctx.run();

    {
        std::lock_guard lck(global_stream_lock);
        std::cout << "thread#" << std::this_thread::get_id() << " " << "End.\n";
    }
}

size_t fac(size_t n)
{
    if (1 < n)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return n * fac(n - 1);
    }
    
    return n;

}

void CalculateFactorial(size_t n)
{
    {
        std::lock_guard lck(global_stream_lock);
        std::cout << "thread#" << std::this_thread::get_id() << " " << "Calculating " << n << "! factorial\n";
    }

    size_t f = fac(n);

    {
        std::lock_guard lck(global_stream_lock);
        std::cout << "thread#" << std::this_thread::get_id() << " " << n << "! = " << f << '\n';
    }
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

    io_ctx.post([func=&CalculateFactorial, n=5]() { func(n); });
    io_ctx.post([func=&CalculateFactorial, n=6]() { func(n); });
    io_ctx.post([func=&CalculateFactorial, n=7]() { func(n); });
    
    worker.reset();
    for(auto &&th : threads) if (th.joinable()) th.join();

    return 0;
}
