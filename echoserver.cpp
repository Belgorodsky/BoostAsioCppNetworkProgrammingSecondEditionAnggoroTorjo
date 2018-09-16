/* echoserver.cpp */
#include "wrapper.h"
#include <boost/current_function.hpp>
#include <iostream>
#include <mutex>
#include <thread>
#include <algorithm>
#include <type_traits>

std::mutex global_stream_lock;

class MyConnection : public Connection
{
public:
    MyConnection(std::shared_ptr<Hive> hive) :
        Connection(hive) 
    {
    }

    ~MyConnection() override = default;

private:
    void OnAccept(const std::string &host, uint16_t port) override
    {
        {
            std::lock_guard lck(global_stream_lock);
            std::cout << "Thread#" << std::this_thread::get_id() << ' '
                      << BOOST_CURRENT_FUNCTION
                      << ' ' << host << ':' << port << '\n';
        }

        Recv();
    }

    void OnConnect(const std::string &host, uint16_t port) override
    {
        {
            std::lock_guard lck(global_stream_lock);
            std::cout << "Thread#" << std::this_thread::get_id() << ' '
                      << BOOST_CURRENT_FUNCTION
                      << ' ' << host << ':' << port << '\n';
        }

        Recv();
    }

    void OnSend(const std::vector<uint8_t> &buffer) override
    {
        std::lock_guard lck(global_stream_lock);
        std::cout << "Thread#" << std::this_thread::get_id() << ' '
                  << BOOST_CURRENT_FUNCTION
                  << ' ' << buffer.size() << " bytes to "
                  << GetSocket().remote_endpoint() << ": ";

        for (size_t x = 0; x < buffer.size(); ++x)
        {
            if (isprint(buffer[x]) && !iscntrl(buffer[x]))
                std::cout  << buffer[x] << ' ';
            else
                std::cout << std::hex << "0x" << (int)buffer[x] << ' ';

            if (x && (x) % 16 == 0)
                std::cout << '\n';
        }

        std::cout << std::endl;
    }

    void OnRecv(std::vector<uint8_t> &buffer) override
    {
        {
            std::lock_guard lck(global_stream_lock);
            std::cout << "Thread#" << std::this_thread::get_id() << ' '
                      << BOOST_CURRENT_FUNCTION
                      << ' ' << buffer.size() << " bytes from "
                      << GetSocket().remote_endpoint() << ": ";

            for (size_t x = 0; x < buffer.size(); ++x)
            {
                if (isprint(buffer[x]) && !iscntrl(buffer[x]))
                    std::cout  << buffer[x] << ' ';
                else
                    std::cout << std::hex << "0x" << (int)buffer[x] << ' ';

                if (x && (x) % 16 == 0)
                    std::cout << '\n';
            }

            std::cout << std::endl;
        }

        // Start the next receive
        Recv();

        // Echo the data back
        Send(std::move(buffer));
    }

    void OnTimer(const boost::posix_time::time_duration &delta) override
    {
        std::lock_guard lck(global_stream_lock);
        std::cout << "Thread#" << std::this_thread::get_id() << ' '
                  << BOOST_CURRENT_FUNCTION << ' '
		  << GetSocket().remote_endpoint()
                  << ' ' << delta << '\n';
    }

    void OnError(const boost::system::error_code &error) override
    {
        std::lock_guard lck(global_stream_lock);
	std::cout << std::flush;
        std::cout << "Thread#" << std::this_thread::get_id() << ' '
                  << BOOST_CURRENT_FUNCTION 
                  << ' ' << error << '\n' << std::flush;
    }
};

class MyAcceptor : public Acceptor
{
public:
    MyAcceptor(std::shared_ptr<Hive> hive) : 
        Acceptor(hive)
    {}

    ~MyAcceptor() override = default;

    void Accept()
    {
        auto connection = std::make_shared<MyConnection>(GetHive());
        Acceptor::Accept(connection);
    }

private:
    bool OnAccept(
        std::shared_ptr<Connection> connection,
        const std::string &host,
        uint16_t port
    ) override
    {
        {
            std::lock_guard lck(global_stream_lock);
            std::cout << "Thread#" << std::this_thread::get_id() << ' '
                      << BOOST_CURRENT_FUNCTION
                          << ' ' << host << ':' << port << '\n';
        }
        auto new_conn = std::make_shared<MyConnection>(GetHive());
        Acceptor::Accept(new_conn);
        return true;
    }

    void OnTimer(const boost::posix_time::time_duration &delta) override
    {
        std::lock_guard lck(global_stream_lock);
        std::cout << "Thread#" << std::this_thread::get_id() << ' '
                  << BOOST_CURRENT_FUNCTION
                  << ' ' << delta << '\n';
    }

    void OnError(const boost::system::error_code &error) override
    {
        std::lock_guard lck(global_stream_lock);
        std::cout << "Thread#" << std::this_thread::get_id() << ' '
                  << BOOST_CURRENT_FUNCTION
                  << ' ' << error << '\n';
    }
};


void WorkerThread(boost::asio::io_context &ioctx, size_t counter)
{
    {
        std::lock_guard lck(global_stream_lock);
        std::cout << "Thread#" << std::this_thread::get_id() << ' '
                  << BOOST_CURRENT_FUNCTION << ' '
                  << counter << " Start.\n";
    }

    while (true)
    {
        try
        {
            boost::system::error_code ec;
            ioctx.run(ec);
            if (ec)
            {
                std::lock_guard lck(global_stream_lock);
                std::cout << "Thread#" << std::this_thread::get_id() << ' '
                          << BOOST_CURRENT_FUNCTION
                          << " Error message: " << ec << ".\n";
            }
            break;
        }
        catch (std::exception &e)
        {
            std::lock_guard lck(global_stream_lock);
            std::cout << "Thread#" << std::this_thread::get_id() << ' '
                      << BOOST_CURRENT_FUNCTION
                      << " Exception message: " << e.what() << ".\n";
        }
    }

    std::lock_guard lck(global_stream_lock);
    std::cout << "Thread#" << std::this_thread::get_id() << ' '
              << BOOST_CURRENT_FUNCTION << ' '
              << counter << " End.\n";
}

int main()
{
    std::cout << "Thread#" << std::this_thread::get_id() << ' '
              << BOOST_CURRENT_FUNCTION
              << " Press ENTER to exit!\n";

    auto hive = std::make_shared<Hive>();

    auto acceptor = std::make_shared<MyAcceptor>(hive);
    acceptor->Listen("127.0.0.1", 4444);

    acceptor->Accept();

    auto threads_count = std::thread::hardware_concurrency();
    std::vector<std::thread> threads(threads_count);
    {
        size_t i = 0;
        std::transform(
            threads.cbegin(),
            threads.cend(),
            threads.begin(),
            [&n=i,&io=hive->GetContext()](auto &&th)
            {
                ++n;
                return std::decay_t<decltype(th)>(&WorkerThread, std::ref(io), n);
            }
        );
    }
    
    std::cin.get();

    boost::system::error_code ec;
    acceptor->GetAcceptor().close(ec);

    hive->Stop();

    for (auto &&th : threads)
    {
        if (th.joinable())
            th.join();
    }
    
    std::cout << "Thread#" << std::this_thread::get_id() << ' '
              << BOOST_CURRENT_FUNCTION
              << " Exit caused by press ENTER!\n";

    return 0;
}
