/* wrapper.cpp */
#include "wrapper.h"
#include <boost/bind.hpp>
#include <charconv>
#include <system_error>

// Hive::GetContext definition
boost::asio::io_context &Hive::GetContext()
{
	return m_io_context;
}

// Hive::HasStopped definition
bool Hive::HasStopped()
{
	return m_shutdown;
}

// Hive::Poll definition
void Hive::Poll()
{
	m_io_context.poll();
}

// Hive::Run definition
void Hive::Run()
{
	m_io_context.run();
}

// Hive::Stop definition
void Hive::Stop()
{
    bool cmp = false; // expected value for compare 
    constexpr bool with = true; // new value to swap with
	if (m_shutdown.compare_exchange_weak(cmp,with) || false == cmp )
	{
		m_work_ptr.reset();
		m_io_context.run();
		m_io_context.stop();
	}
}

// Hive::Reset definition
void Hive::Reset()
{
    bool cmp = true; // expected value for compare 
    constexpr bool with = false; // new value to swap with
	if (m_shutdown.compare_exchange_weak(cmp,with) || true == cmp)
	{
		m_io_context.reset();
        m_work_ptr = std::make_unique<work_type>(boost::asio::make_work_guard(m_io_context));
	}
}

// Acceptor constructor 
Acceptor::Acceptor(std::shared_ptr<Hive> hive) :
    m_hive(hive), 
    m_acceptor(m_hive->GetContext()), 
    m_io_strand(m_hive->GetContext()), 
    m_timer(m_hive->GetContext())
{
}

// Acceptor::StartTimer definition
void Acceptor::StartTimer()
{
	m_last_time = boost::posix_time::microsec_clock::local_time();
	m_timer.expires_from_now(boost::posix_time::milliseconds(m_timer_interval));
	m_timer.async_wait(
        boost::asio::bind_executor(
            m_io_strand,
            [self=shared_from_this()](auto &&ec) mutable
            {
                self->HandleTimer(ec);
            }
        )
    );
}

// Acceptor::StartError definition
void Acceptor::StartError( const boost::system::error_code &error)
{
    bool cmp = false; // expected value for compare 
    constexpr bool with = true; // new value to swap with
    if (m_error_state.compare_exchange_weak(cmp, with) || false == cmp)
	{
		boost::system::error_code ec;
		m_acceptor.cancel(ec);
		m_acceptor.close(ec);
		m_timer.cancel(ec);
		OnError(error);
	}
}

// Acceptor::DispatchAccept definition
void Acceptor::DispatchAccept(std::shared_ptr<Connection> connection)
{
	m_acceptor.async_accept(
        connection->GetSocket(),
        boost::asio::bind_executor(
            connection->GetStrand(),
            [self=shared_from_this(),con=connection](auto &&ec) mutable
            {
                self->HandleAccept(ec,con);
            }
        )
    );
}

// Acceptor::HandleTimer definition
void Acceptor::HandleTimer(const boost::system::error_code &error)
{
	if (error || HasError() || m_hive->HasStopped())
    {
		StartError(error);
    }
	else
	{
		OnTimer(boost::posix_time::microsec_clock::local_time() - m_last_time);
		StartTimer();
	}
}

// Acceptor::HandleAccept definition
void Acceptor::HandleAccept(const boost::system::error_code &error, std::shared_ptr<Connection> connection)
{
	if (error || HasError() || m_hive->HasStopped())
    {
		connection->StartError(error);
    }
	else
	{
		if (connection->GetSocket().is_open())
		{
			connection->StartTimer();
			if (
                OnAccept(
                    connection,
                    connection->GetSocket().remote_endpoint().address().to_string(),
                    connection->GetSocket().remote_endpoint().port()
                )
            )
			{
				connection->OnAccept(
                    m_acceptor.local_endpoint().address().to_string(),
                    m_acceptor.local_endpoint().port()
                );
			}
		}
		else
        {
			StartError(error);
        }
	}
}

// Acceptor::Stop definition
void Acceptor::Stop()
{
    boost::asio::post(
        m_io_strand,
        [self=shared_from_this()]()
        {
            self->HandleTimer(boost::asio::error::connection_reset);
        }
	);
}

// Acceptor::Accept definition
void Acceptor::Accept(std::shared_ptr<Connection> connection)
{
    boost::asio::post(
        m_io_strand,
        [self=shared_from_this(),conn=connection]() mutable
        {
            self->DispatchAccept(conn);
        }
    );
}

// Acceptor::Listen definition
void Acceptor::Listen(const std::string &host, const uint16_t &port)
{
	boost::asio::ip::tcp::resolver resolver(m_hive->GetContext());
    constexpr size_t max_port_length_with_zero_term = 6u;
    std::array<char, max_port_length_with_zero_term> port_str = {0};
    std::to_chars(port_str.data(), port_str.data() + port_str.size(), port);
	boost::asio::ip::tcp::resolver::query query(host, port_str.data());
	boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
	m_acceptor.open(endpoint.protocol());
	m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	m_acceptor.bind(endpoint);
	m_acceptor.listen(boost::asio::socket_base::max_connections);
	StartTimer();
}

// Acceptor::GetHive definition
std::shared_ptr<Hive> Acceptor::GetHive()
{
	return m_hive;
}

// Acceptor::GetAcceptor definition
boost::asio::ip::tcp::acceptor &Acceptor::GetAcceptor()
{
	return m_acceptor;
}

// Acceptor::GetTimerInterval definition
int32_t Acceptor::GetTimerInterval() const
{
	return m_timer_interval;
}

// Acceptor::SetTimerInterval definition
void Acceptor::SetTimerInterval(int32_t timer_interval)
{
	m_timer_interval = timer_interval;
}

// Acceptor::HasError definition
bool Acceptor::HasError()
{
	return m_error_state;
}

// Connection constructor
Connection::Connection(std::shared_ptr<Hive> hive) :
    m_hive(hive),
    m_socket(m_hive->GetContext()),
    m_io_strand(m_hive->GetContext()),
    m_timer(m_hive->GetContext())
{
}

// Connection::Bind definition
void Connection::Bind(const std::string &ip, uint16_t port)
{
	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(ip), port);
	m_socket.open(endpoint.protocol());
	m_socket.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	m_socket.bind(endpoint);
}

// Connection::StartSend definition
void Connection::StartSend()
{
	if (!m_pending_sends.empty())
	{
		boost::asio::async_write(
            m_socket,
            boost::asio::buffer(m_pending_sends.front()),
            boost::asio::bind_executor(
                m_io_strand,
                [
                    self=shared_from_this(),
                    send_buffer_it=m_pending_sends.begin()
                ] (auto &&ec, auto &&...)
                {
                    self->HandleSend(ec,send_buffer_it);
                }
            )
        );
	}
}

// Connection::StartRecv definition
void Connection::StartRecv(int32_t total_bytes)
{
	if(total_bytes > 0)
	{
		m_recv_buffer.resize(total_bytes);
		boost::asio::async_read(
            m_socket,
            boost::asio::buffer(m_recv_buffer),
            boost::asio::bind_executor(
                m_io_strand,
                [self=shared_from_this()] (auto &&ec, auto &&bytes)
                {
                    self->HandleRecv(ec, bytes);
                }
            )
        );
	}
	else
	{
		m_recv_buffer.resize(m_receive_buffer_size);
		m_socket.async_read_some(
            boost::asio::buffer(m_recv_buffer), 
            boost::asio::bind_executor(
                m_io_strand,
                [self=shared_from_this()] (auto &&ec, auto &&bytes)
                {
                    self->HandleRecv(ec, bytes);
                }
            )
        );
	}
}

// Connection::StartTimer definition
void Connection::StartTimer()
{
	m_last_time = boost::posix_time::microsec_clock::local_time();
	m_timer.expires_from_now(boost::posix_time::milliseconds(m_timer_interval));
	m_timer.async_wait(
        boost::asio::bind_executor(
            m_io_strand,
            [self=shared_from_this()] (auto &&ec)
            {
                self->DispatchTimer(ec);
            }
        )
    );
}

// Connection::StartError definition
void Connection::StartError(const boost::system::error_code &error)
{
    bool cmp = false; // expected value for compare 
    constexpr bool with = true; // new value to swap with
    if (m_error_state.compare_exchange_weak(cmp, with) || false == cmp)
	{
		boost::system::error_code ec;
		m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		m_socket.close(ec);
		m_timer.cancel(ec);
		OnError(error);
	}
}

// Connection::HandleConnect definition
void Connection::HandleConnect(const boost::system::error_code &error)
{
	if(error || HasError() || m_hive->HasStopped())
    {
		StartError( error );
    }
	else
	{
		if(m_socket.is_open())
			OnConnect( m_socket.remote_endpoint().address().to_string(), m_socket.remote_endpoint().port() );
		else
			StartError( error );
	}
}

// Connection::HandleSend definition
void Connection::HandleSend(const boost::system::error_code &error, std::list<std::vector<uint8_t> >::iterator itr)
{
	if(error || HasError() || m_hive->HasStopped())
    {
		StartError(error);
    }
	else
	{
		OnSend(*itr);
		m_pending_sends.erase(itr);
		StartSend();
	}
}

// Connection::HandleRecv definition
void Connection::HandleRecv(const boost::system::error_code &error, int32_t actual_bytes)
{

	if(error || HasError() || m_hive->HasStopped())
    {
		StartError( error );
    }
	else
	{
		m_recv_buffer.resize(actual_bytes);
		OnRecv(m_recv_buffer);
		m_pending_recvs.pop_front();
		if(!m_pending_recvs.empty())
			StartRecv( std::move(m_pending_recvs.front()) );
	}
}

// Connection::HandleTimer definition
void Connection::HandleTimer(const boost::system::error_code &error)
{
	if(error || HasError() || m_hive->HasStopped())
    {
		StartError( error );
    }
	else
	{
		OnTimer(boost::posix_time::microsec_clock::local_time() - m_last_time);
		StartTimer();
	}
}

// Connection::DispatchSend definition
void Connection::DispatchSend(std::vector<uint8_t> &&buffer)
{
	bool should_start_send = m_pending_sends.empty();
	m_pending_sends.emplace_back(std::move(buffer));
	if(should_start_send)
		StartSend();
}

// Connection::DispatchRecv definition
void Connection::DispatchRecv(int32_t total_bytes)
{
	bool should_start_receive = m_pending_recvs.empty();
	m_pending_recvs.push_back(total_bytes);
	if(should_start_receive)
		StartRecv(total_bytes);
}

// Connection::DispatchTimer definition
void Connection::DispatchTimer(const boost::system::error_code &error)
{
    boost::asio::post(
        m_io_strand,
        [self=shared_from_this(),ec=error]()
        {
            self->HandleTimer(ec);
        }
    );
}

// Connection::Connect definition
void Connection::Connect(const std::string & host, uint16_t port)
{
	boost::system::error_code ec;
	boost::asio::ip::tcp::resolver resolver(m_hive->GetContext());
    constexpr size_t max_port_length_with_zero_term = 6u;
    std::array<char, max_port_length_with_zero_term> port_str = {0};
    std::to_chars(port_str.data(), port_str.data() + port_str.size(), port);
	boost::asio::ip::tcp::resolver::query query(host, port_str.data());
	boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
	m_socket.async_connect(
        *iterator,
        boost::asio::bind_executor(
            m_io_strand,
            [self=shared_from_this()](auto &&ec)
            {
                self->HandleConnect(ec);
            }
        )
    );
	StartTimer();
}

// Connection::Disconnect definition
void Connection::Disconnect()
{
    boost::asio::post(
        m_io_strand,
        [self=shared_from_this()]()
        {
            self->HandleTimer(boost::asio::error::connection_reset);
        }
    );
}

// Connection::Recv definition
void Connection::Recv(int32_t total_bytes)
{
    boost::asio::post(
        m_io_strand,
        [self=shared_from_this(),bytes=total_bytes]()
        {
            self->DispatchRecv(bytes);
        }
    );
}

// Connection::Send definition
void Connection::Send(const std::vector<uint8_t> &buffer)
{
    boost::asio::post(
        m_io_strand,
        [self=shared_from_this(),buf=buffer]() mutable
        {
            self->DispatchSend(std::move(buf));
        }
	);
}

// Connection::Send definition with move semantics
void Connection::Send(std::vector<uint8_t> &&buffer)
{
    boost::asio::post(
        m_io_strand,
        [self=shared_from_this(),buf=std::move(buffer)]() mutable
        {
            self->DispatchSend(std::move(buf));
        }
	);
}

// Connection::GetSocket definition
boost::asio::ip::tcp::socket &Connection::GetSocket()
{
	return m_socket;
}

// Connection::GetStrand definition
boost::asio::io_context::strand &Connection::GetStrand()
{
	return m_io_strand;
}

// Connection::GetHive definition
std::shared_ptr<Hive> Connection::GetHive()
{
	return m_hive;
}

// Connection::SetReceiveBufferSize definition
void Connection::SetReceiveBufferSize(int32_t size)
{
	m_receive_buffer_size = size;
}

// Connection::GetReceiveBufferSize definition
int32_t Connection::GetReceiveBufferSize() const
{
	return m_receive_buffer_size;
}

// Connection::GetTimerInterval definition
int32_t Connection::GetTimerInterval() const
{
	return m_timer_interval;
}

// Connection::SetTimerInterval definition
void Connection::SetTimerInterval(int32_t timer_interval)
{
	m_timer_interval = timer_interval;
}

// Connection::HasError definition
bool Connection::HasError()
{
	return m_error_state;
}
