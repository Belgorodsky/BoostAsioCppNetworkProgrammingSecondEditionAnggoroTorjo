/* wrapper.h */
#ifndef _WRAPPER_H_
#define _WRAPPER_H_

// Include the required header files
#include <boost/asio.hpp>
#include <boost/current_function.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <memory>
#include <string>
#include <vector>
#include <list>
#include <cstdint>
#include <atomic>

// Class declaration
class Hive;
class Acceptor;
class Connection;

// Class Hive definition and its members declaration
class Hive : public std::enable_shared_from_this<Hive>
{
public:
	Hive() = default;
	virtual ~Hive() = default;

	Hive(const Hive & rhs) = delete;
	Hive & operator =(const Hive & rhs) = delete;

	// Returns the io_context of this object.
	boost::asio::io_context& GetContext();

	// Returns true if the Stop function has been called.
	bool HasStopped();

	// Polls the networking subsystem once from the current thread and 
	// returns.
	void Poll();

	// Runs the networking system on the current thread. This function blocks 
	// until the networking system is stopped, so do not call on a single 
	// threaded application with no other means of being able to call Stop 
	// unless you code in such logic.
	void Run();

	// Stops the networking system. All work is finished and no more 
	// networking interactions will be possible afterwards until Reset is called.
	void Stop();

	// Restarts the networking system after Stop as been called. A new work
	// object is created ad the shutdown flag is cleared.
	void Reset();

private:
    boost::asio::io_context m_io_context;
    using work_type = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
    std::unique_ptr<work_type> m_work_ptr{std::make_unique<work_type>(boost::asio::make_work_guard(m_io_context))};
    std::atomic<bool> m_shutdown{false};
};

// Class Acceptor definition and its members declaration
class Acceptor : public std::enable_shared_from_this<Acceptor>
{
	friend class Hive;

public:
    Acceptor(const Acceptor &rhs) = delete;
    Acceptor& operator=(const Acceptor &rhs) = delete;

	// Returns the Hive object.
	std::shared_ptr<Hive> GetHive();

	// Returns the acceptor object.
	boost::asio::ip::tcp::acceptor &GetAcceptor();

	// Returns the strand object.
	boost::asio::io_context::strand &GetStrand();

	// Sets the timer interval of the object. The interval is changed after 
	// the next update is called. The default value is 1000 ms.
	void SetTimerInterval(int32_t timer_interval_ms);

	// Returns the timer interval of the object.
	int32_t GetTimerInterval() const;

	// Returns true if this object has an error associated with it.
	bool HasError();

	// Begin listening on the specific network interface.
	void Listen(const std::string &host, const uint16_t &port);

	// Posts the connection to the listening interface. The next client that
	// connections will be given this connection. If multiple calls to Accept
	// are called at a time, then they are accepted in a FIFO order.
	void Accept(std::shared_ptr<Connection> connection);

	// Stop the Acceptor from listening.
	void Stop();

protected:
	Acceptor(std::shared_ptr<Hive> hive);
	virtual ~Acceptor() = default;

private:
	void StartTimer();
	void StartError(const boost::system::error_code & error);
	void DispatchAccept(std::shared_ptr<Connection> connection);
	void HandleTimer(const boost::system::error_code & error);
	void HandleAccept(const boost::system::error_code & error, std::shared_ptr<Connection> connection);
	// Called when a connection has connected to the server. This function 
	// should return true to invoke the connection's OnAccept function if the 
	// connection will be kept. If the connection will not be kept, the 
	// connection's Disconnect function should be called and the function 
	// should return false.
	virtual bool OnAccept(
        std::shared_ptr<Connection> connection,
        const std::string &host,
        uint16_t port
    ) = 0;

	// Called on each timer event.
	virtual void OnTimer(const boost::posix_time::time_duration &delta) = 0;

	// Called when an error is encountered. Most typically, this is when the
	// acceptor is being closed via the Stop function or if the Listen is 
	// called on an address that is not available.
	virtual void OnError(const boost::system::error_code &error) = 0;

private:
	std::shared_ptr<Hive> m_hive;
	boost::asio::ip::tcp::acceptor m_acceptor;
	boost::asio::io_context::strand m_io_strand;
	boost::asio::deadline_timer m_timer;
	boost::posix_time::ptime m_last_time;
    int32_t m_timer_interval{1000};
    std::atomic<bool> m_error_state{false};
};

// Class Connection definition and its members declaration
class Connection : public std::enable_shared_from_this<Connection>
{
	friend class Acceptor;
	friend class Hive;

public:
	Connection(const Connection &rhs) = delete;
	Connection& operator=(const Connection &rhs) = delete;

	// Returns the Hive object.
	std::shared_ptr<Hive> GetHive();

	// Returns the socket object.
	boost::asio::ip::tcp::socket &GetSocket();

	// Returns the strand object.
	boost::asio::io_context::strand &GetStrand();

	// Sets the application specific receive buffer size used. For stream 
	// based protocols such as HTTP, you want this to be pretty large, like 
	// 64kb. For packet based protocols, then it will be much smaller, 
	// usually 512b - 8kb depending on the protocol. The default value is
	// 4kb.
	void SetReceiveBufferSize(int32_t size);

	// Returns the size of the receive buffer size of the current object.
	int32_t GetReceiveBufferSize() const;

	// Sets the timer interval of the object. The interval is changed after 
	// the next update is called.
	void SetTimerInterval(int32_t timer_interval_ms);

	// Returns the timer interval of the object.
	int32_t GetTimerInterval() const;

	// Returns true if this object has an error associated with it.
	bool HasError();

	// Binds the socket to the specified interface.
	void Bind(const std::string &ip, uint16_t port);

	// Starts an a/synchronous connect.
	void Connect(const std::string &host, uint16_t port);

	// Posts data to be sent to the connection.
	void Send(const std::vector<uint8_t> &buffer);

	// Posts data to be sent to the connection with move semantics.
	void Send(std::vector<uint8_t> &&buffer);

	// Posts a recv for the connection to process. If total_bytes is 0, then 
	// as many bytes as possible up to GetReceiveBufferSize() will be 
	// waited for. If Recv is not 0, then the connection will wait for exactly
	// total_bytes before invoking OnRecv.
	void Recv(int32_t total_bytes = 0);

	// Posts an asynchronous disconnect event for the object to process.
	void Disconnect();

protected:
	Connection(std::shared_ptr<Hive> hive);
	virtual ~Connection() = default;

private:
	void StartSend();
	void StartRecv(int32_t total_bytes);
	void StartTimer();
	void StartError(const boost::system::error_code &error);
	void DispatchSend(std::vector<uint8_t> &&buffer);
	void DispatchRecv(int32_t total_bytes);
	void DispatchTimer(const boost::system::error_code &error);
	void HandleConnect(const boost::system::error_code &error);
	void HandleSend(const boost::system::error_code &error, std::list<std::vector<uint8_t> >::iterator itr);
	void HandleRecv(const boost::system::error_code &error, int32_t actual_bytes );
	void HandleTimer(const boost::system::error_code &error);

	// Called when the connection has successfully connected to the local
	// host.
	virtual void OnAccept(const std::string &host, uint16_t port) = 0;

	// Called when the connection has successfully connected to the remote
	// host.
	virtual void OnConnect(const std::string &host, uint16_t port) = 0;

	// Called when data has been sent by the connection.
	virtual void OnSend(const std::vector<uint8_t> &buffer) = 0;

	// Called when data has been received by the connection. 
	virtual void OnRecv(std::vector<uint8_t> &buffer ) = 0;

	// Called on each timer event.
	virtual void OnTimer(const boost::posix_time::time_duration &delta) = 0;

	// Called when an error is encountered.
	virtual void OnError(const boost::system::error_code &error) = 0;

private:
    std::shared_ptr<Hive> m_hive;
	boost::asio::ip::tcp::socket m_socket;
	boost::asio::io_context::strand m_io_strand;
	boost::asio::deadline_timer m_timer;
	boost::posix_time::ptime m_last_time;
	std::vector<uint8_t> m_recv_buffer;
	std::list<int32_t> m_pending_recvs;
	std::list<std::vector<uint8_t> > m_pending_sends;
	int32_t m_receive_buffer_size{4096};
	int32_t m_timer_interval{1000};
	std::atomic<bool> m_error_state{false};
};
#endif // _WRAPPER_H_
