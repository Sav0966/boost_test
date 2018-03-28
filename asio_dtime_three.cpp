// asio_dtime_three.cpp: An asynchronous TCP daytime server
//
// This tutorial program shows how to use asio
// to implement a server application with TCP

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using boost::asio::ip::tcp;

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

int main()
{
	try
	{
		// We need to create a server object to accept incoming client
		// connections. The boost::asio::io_context object provides I/O
		// services, such as sockets, that the server object will use
		boost::asio::io_context io_context;
		tcp_server server(io_context);

		io_context.run(); // Run the boost::asio::io_service object so
		// that it will perform asynchronous operations on your behalf
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
