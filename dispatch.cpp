/* dispatch.cpp */
#include <boost/asio.hpp>
#include <thread>
#include <iostream>
#include <mutex>
#include <chrono>

std::mutex global_stream_lock;

void WorkerThread(
    boost::asio::io_service &iosvc
)
{
    {
        // c++17 code, if not compile write std::lock_guard<std::mutex> lck(global_stream_lock); 
        std::lock_guard lck(global_stream_lock);
        std::cout << "thread#" << std::this_thread::get_id() << " Thread Start.\n";
    }

    iosvc.run();

    {
        std::lock_guard lck(global_stream_lock);
        std::cout << "thread#" << std::this_thread::get_id() << " Thread Finish.\n";
    }
}

void Dispatch(size_t i)
{
    std::lock_guard lck(global_stream_lock);
    std::cout << "thread#" << std::this_thread::get_id() << " " << "dispatch() for i = " << i << '\n';
}

void Post(size_t i)
{
    std::lock_guard lck(global_stream_lock);
    std::cout << "thread#" << std::this_thread::get_id() << " " << "post() for i = " << i << '\n';
}

void Running(boost::asio::io_service &iosvc)
{
    for (size_t x = 0; x != 5; ++x)
    {
        iosvc.dispatch([fn=&Dispatch, i=x](){fn(i);});
        iosvc.post    ([fn=&Post, i=x](){fn(i);});
    }
}

int main()
{
    boost::asio::io_service io_svc;
    auto worker = std::make_unique<boost::asio::io_service::work>(io_svc);
    {
        std::lock_guard lck(global_stream_lock);
        std::cout << "thread#" << std::this_thread::get_id() << " " << "The program will exit once all work has finished.\n";
    }

    std::thread th(WorkerThread, std::ref(io_svc));

    io_svc.post([fn=&Running, &io=io_svc]() {fn(io);});
    // uncomment below 4 some fun
//    io_svc.run_one();
    worker.reset();
    th.join();

    return 0;
}
