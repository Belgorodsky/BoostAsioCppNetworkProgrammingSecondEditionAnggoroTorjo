/* readwritesocket.cpp */
#include <boost/asio.hpp>
#include <boost/current_function.hpp>
#include <memory>
#include <thread>
#include <mutex>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <type_traits>

std::mutex global_stream_lock;

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

struct ClientContext final : public std::enable_shared_from_this<ClientContext>
{
    boost::asio::ip::tcp::socket m_socket;
    constexpr static size_t m_buffer_size{4096};
    std::vector<uint8_t> m_recv_buffer = std::move(std::vector<uint8_t>(ClientContext::m_buffer_size,0));
    size_t m_recv_buffer_index{0};
    std::list<std::vector<uint8_t>> m_send_buffer;

    ClientContext(const boost::asio::io_context::executor_type& exec) : m_socket{exec.context()} {}
    ClientContext(boost::asio::io_context& io_ctx) : m_socket{io_ctx} {}
    ~ClientContext() = default;

    void Close()
    {
        boost::system::error_code ec;
        m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        m_socket.close(ec);
    }

    template<class It>
    void OnSend(const boost::system::error_code &ec, It it)
    {
        if (ec)
        {
            std::lock_guard lck(global_stream_lock);
            std::cout << "Thread#" << std::this_thread::get_id() << ' '
                      << BOOST_CURRENT_FUNCTION
                      << " Error message: " << ec << " cannot send data to "
                      << m_socket.remote_endpoint() << ".\n";
            Close();
        }
        else
        {
            std::lock_guard lck(global_stream_lock);
            std::cout << "Thread#" << std::this_thread::get_id() << ' '
                      << BOOST_CURRENT_FUNCTION
                      << " Sent " << it->size() << " bytes to "
                      << m_socket.remote_endpoint() << ".\n";
        }
        m_send_buffer.erase(it);

        // Start the next pending send
        if (!m_send_buffer.empty())
        {
            StartNextPandingSend();
        }
    }

    void Send(const void *buffer, size_t length)
    {
        bool can_send_now = false;

        std::vector<uint8_t> output;
        output.reserve(length);
        const uint8_t *from = static_cast<const uint8_t*>(buffer);
        const uint8_t *to = static_cast<const uint8_t*>(from + length);

        std::copy(from, to, std::back_inserter(output));

        // Store if this is the only current send or not
        can_send_now = m_send_buffer.empty();
        
        m_send_buffer.emplace_back(std::move(output));

        // Only send if there are no more pending buffers waiting
        if (can_send_now)
        {
            // Start the next pending send
            StartNextPandingSend();
        }
    }

    void OnRecv(const boost::system::error_code &ec, size_t bytes_transferred)
    {
        if (ec)
        {
            std::lock_guard lck(global_stream_lock);
            std::cout << "Thread#" << std::this_thread::get_id() << ' '
                      << BOOST_CURRENT_FUNCTION
                      << " Error message: " << ec << " cannot receive data from "
                      << m_socket.remote_endpoint() << ".\n";
            Close();
            return;
        }
        else
        {
            // Debug information
            boost::asio::ip::tcp::endpoint remote_ep = m_socket.remote_endpoint();
            std::lock_guard lck(global_stream_lock);
            std::cout << "Thread#" << std::this_thread::get_id() << ' '
                      << BOOST_CURRENT_FUNCTION << ' '
                      << bytes_transferred << " bytes from " << remote_ep << ".\n";

            m_recv_buffer_index += bytes_transferred;

            for (size_t x = 0; x < m_recv_buffer_index; ++x)
            {
                if (isprint(m_recv_buffer[x]) && !iscntrl(m_recv_buffer[x]))
                    std::cout  << m_recv_buffer[x] << ' ';
                else
                    std::cout << std::hex << "0x" << (int)m_recv_buffer[x] << ' ';

                if (x && (x) % 16 == 0)
                    std::cout << '\n';
            }

            std::cout << std::endl;

            // Clear all the data
            m_recv_buffer_index = 0;

            Recv();
        }
    }

    void Recv()
    {
        m_socket.async_read_some(
            boost::asio::buffer(
                &m_recv_buffer[m_recv_buffer_index],
                m_recv_buffer.size() - m_recv_buffer_index
            ),
            [self=shared_from_this()](auto &&ec, auto &&bytes_transferred)
            {
                self->OnRecv(ec, bytes_transferred);
            }
        );
    }
    
private:
    void inline StartNextPandingSend()
    {
            boost::asio::async_write(
                m_socket,
                boost::asio::buffer(m_send_buffer.front()),
                [self=shared_from_this(),it=m_send_buffer.begin()](auto &&ec, auto&&...)
                {
                    self->OnSend(ec, it);
                }
            );
    }
};

void OnAccept(
    boost::asio::ip::tcp::acceptor &ac,
    const boost::system::error_code &ec,
    std::shared_ptr<ClientContext> clnt
)
{
    std::lock_guard lck(global_stream_lock);
    if (ec)
    {
        std::cout << "Thread#" << std::this_thread::get_id() << ' '
                  << BOOST_CURRENT_FUNCTION
                  << " Error message: " << ec << ".\n";
    }
    else
    {
        auto remote_ep = clnt->m_socket.remote_endpoint();
        std::cout << "Thread#" << std::this_thread::get_id() << ' '
                  << BOOST_CURRENT_FUNCTION
                  << " Accepted connection from " << remote_ep << "!\n";
        auto new_client = std::make_shared<ClientContext>(ac.get_executor());
        auto &&sckt = new_client->m_socket;
        ac.async_accept(
            sckt,
            [cl=std::move(new_client),&ac=ac](auto &&ec) mutable
            {
                OnAccept(std::ref(ac), ec, cl);
            }
        );

        std::string_view msg("Hi there!\n");
        clnt->Send(msg.data(), msg.length());
        clnt->Recv();
    }

}

int main()
{
    boost::asio::io_context io_ctx;
    auto worker = boost::asio::make_work_guard(io_ctx);

    {
        std::lock_guard lck(global_stream_lock);
        std::cout << "Thread#" << std::this_thread::get_id() << ' '
                  << BOOST_CURRENT_FUNCTION
                  << " Press ENTER to exit!\n";
    }

    auto threads_count = std::thread::hardware_concurrency();
    std::vector<std::thread> threads(threads_count);
    {
        size_t i = 0;
        std::transform(
            threads.cbegin(),
            threads.cend(),
            threads.begin(),
            [&n=i,&io=io_ctx](auto &&th)
            {
                ++n;
                return std::decay_t<decltype(th)>(&WorkerThread, std::ref(io), n);
            }
        );
    }
    
    boost::asio::ip::tcp::acceptor acceptor(io_ctx);

    try
    {
        auto client = std::make_shared<ClientContext>(io_ctx); 
        boost::asio::ip::tcp::resolver resolver(io_ctx);
        boost::asio::ip::tcp::resolver::query query("127.0.0.1", "4444");
        boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
        acceptor.open(endpoint.protocol());
        acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor.bind(endpoint);
        acceptor.listen(boost::asio::socket_base::max_connections);
        auto &&socket = client->m_socket;
        acceptor.async_accept(
            socket,
            [cl=std::move(client),&ac=acceptor](auto &&ec) mutable
            {
                OnAccept(std::ref(ac), ec, cl);
            }
        );
        {
            std::lock_guard lck(global_stream_lock);
            std::cout << "Thread#" << std::this_thread::get_id() << ' '
                      << BOOST_CURRENT_FUNCTION
                      << " Listening on: " << endpoint << ".\n";
        }
    }
    catch (std::exception &e)
    {
        std::lock_guard lck(global_stream_lock);
        std::cout << "Thread#" << std::this_thread::get_id() << ' '
                  << BOOST_CURRENT_FUNCTION
                  << " Exception message: " << e.what() << ".\n";
    }

    std::cin.get();

    boost::system::error_code ec;
    acceptor.close(ec);

    io_ctx.stop();

    for (auto &&th : threads)
    {
        if (th.joinable())
            th.join();
    }

    return 0;
}
