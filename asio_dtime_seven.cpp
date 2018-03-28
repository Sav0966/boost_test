// asio_dtime_seven.cpp: A combined TCP/UDP asynchronous server
//
// This tutorial program shows how to combine the two asynchronous
// servers that we have just written, into a single server application

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using boost::asio::ip::tcp;
using boost::asio::ip::udp;

// We define the function make_daytime_string() to create
// the string to be sent back to the client. This function will
// be reused in all of our daytime server applications
std::string make_daytime_string()
{
	using namespace boost::posix_time;

	// Get current universal time and create DATETIME facet
	ptime utc = boost::posix_time::second_clock::universal_time();
	time_facet* facet(new time_facet("%A, %B %d, %Y %H:%M:%S-UTC"));

	std::stringstream ss;
	ss.imbue(std::locale(std::locale::classic(), facet));
	ss << utc; // Thursday, March 22, 2018 13:00:53-UTC

	std::cout << ss.str() << std::endl;
	return ss.str();
}

// The tcp_connection and tcp_server classes are taken from Daytime 3

class tcp_connection
	: public boost::enable_shared_from_this<tcp_connection>
{
public:
	// We will use shared_ptr and enable_shared_from_this
	// because we want to keep the tcp_connection object alive
	// as long as there is an operation that refers to it
	typedef boost::shared_ptr<tcp_connection> pointer;

	static pointer create(boost::asio::io_context& io_context)
	{
		return pointer(new tcp_connection(io_context));
	}

	tcp::socket& socket()
	{
		return socket_;
	}

	void start()
	{
		// The data to be sent is stored in the class member
		// message_ as we need to keep the data valid until
		// the asynchronous operation is complete
		message_ = make_daytime_string();

		// When initiating the asynchronous operation, and if using
		// boost::bind(), you must specify only the arguments that match
		// the handler's parameter list. In this program, both of the
		// argument placeholders (boost::asio::placeholders::error and
		// boost::asio::placeholders::bytes_transferred) could potentially
		// have been removed, since they are not being used in handle_write()
		boost::asio::async_write(socket_, boost::asio::buffer(message_),
			boost::bind(&tcp_connection::handle_write, shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));


		// Any further actions for this client connection
		// are now the responsibility of handle_write()
	}

private:
	// For new() object creation in a create() function
	tcp_connection(boost::asio::io_context& io_context)
		: socket_(io_context) // Initialize socket_
	{
	}

	void handle_write(const boost::system::error_code& /*error*/,
		size_t /*bytes_transferred*/)
	{
		// You may have noticed that the error, and bytes_transferred parameters
		// are not used in the body of the handle_write() function. If parameters
		// are not needed, it is possible to remove them from the function so that
		// it looks like:  void handle_write() {}.  The boost::asio::async_write()
		// call used to initiate the call can then be changed to just:
		//	boost::asio::async_write(socket_, boost::asio::buffer(message_),
		//	boost::bind(&tcp_connection::handle_write, shared_from_this()));
	}

	tcp::socket socket_;
	std::string message_;
};

class tcp_server
{
public:
	// The constructor initialises an acceptor to listen on TCP port 13
	tcp_server(boost::asio::io_context& io_context)
		: acceptor_(io_context, tcp::endpoint(tcp::v4(), 13))
	{
		start_accept();
	}

private:
	void start_accept()
	{
		tcp_connection::pointer new_connection =
			tcp_connection::create(acceptor_.get_executor().context());

		// The function start_accept() creates a socket and initiates
		// an asynchronous accept operation to wait for a new connection

		acceptor_.async_accept(new_connection->socket(),
			boost::bind(&tcp_server::handle_accept, this, new_connection,
			boost::asio::placeholders::error));
	}

	void handle_accept(tcp_connection::pointer new_connection,
		const boost::system::error_code& error)
	{
		// The function handle_accept() is called when the asynchronous
		// accept operation initiated by start_accept() finishes

		if (!error)
		{
			new_connection->start();
		} // It services the client request,

		start_accept(); // and then calls start_accept()
		// to initiate the next accept operation
	}

	tcp::acceptor acceptor_;
};

// The udp_server class  is taken from  Daytime 6 

class udp_server
{
public:
	// The constructor initialises a socket to listen on UDP port 13
	udp_server(boost::asio::io_context& io_context)
		: socket_(io_context, udp::endpoint(udp::v4(), 13))
	{
		start_receive();
	}

private:
	// The function ip::udp::socket::async_receive_from() will cause
	// the application to listen in the background for a new request.
	// When such a request is received, the boost::asio::io_service
	// object will invoke the handle_receive() function with two arguments:
	// a value of type boost::system::error_code indicating whether the
	// operation succeeded or failed, and a size_t value bytes_transferred
	// specifying the number of bytes received
	void start_receive()
	{
		socket_.async_receive_from(
			boost::asio::buffer(recv_buffer_), remote_endpoint_,
			boost::bind(&udp_server::handle_receive, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
	}

	// The function handle_receive() will service the client request
	void handle_receive(const boost::system::error_code& error,
		std::size_t /*bytes_transferred*/)
	{
		// The error parameter contains the result of the asynchronous
		// operation. Since we only provide the 1-byte recv_buffer_ to
		// contain the client's request, the boost::asio::io_service
		// object would return an error if the client sent anything
		// larger. We can ignore such an error if it comes up
		if (!error || error == boost::asio::error::message_size)
		{
			// Determine what we are going to send
			boost::shared_ptr<std::string> message(
				new std::string(make_daytime_string()));

			// We now call ip::udp::socket::async_send_to()
			// to serve the data to the client
			socket_.async_send_to(boost::asio::buffer(*message), remote_endpoint_,
				boost::bind(&udp_server::handle_send, this, message,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));

			// Any further actions for this client request are
			// now the responsibility of handle_send()

			// Start listening for the next client request
			start_receive();
		}
	}

	// The function handle_send() is invoked
	// after the service request has been completed
	void handle_send(boost::shared_ptr<std::string> /*message*/,
		const boost::system::error_code& /*error*/,
		std::size_t /*bytes_transferred*/)
	{
		// When initiating the asynchronous operation, and if using
		// boost::bind(), you must specify only the arguments that
		// match the handler's parameter list. In this program,
		// both of the argument placeholders
		// (boost::asio::placeholders::error and
		// boost::asio::placeholders::bytes_transferred)
		// could potentially have been removed
	}

	udp::socket socket_;
	udp::endpoint remote_endpoint_;
	boost::array<char, 1> recv_buffer_;
};

int main()
{
	try
	{
		boost::asio::io_context io_context;

		// We will begin by creating a server object to accept
		// a TCP client connection. We also need a server object
		// to accept a UDP client request
		tcp_server server1(io_context);
		udp_server server2(io_context);

		// We have created two lots of work for
		// the boost::asio::io_service object to do
		io_context.run();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
