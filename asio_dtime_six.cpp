// asio_dtime_six.cpp: An asynchronous UDP daytime server
//
// This tutorial program shows how to use asio
// to implement a server application with UDP

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

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
		udp_server server(io_context);

		// Create a server object to accept
		// incoming client requests, and run
		// the boost::asio::io_service object
		io_context.run();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
