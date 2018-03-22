// asio_dtime_two.cpp: A synchronous TCP daytime server
//
// This tutorial program shows how to use asio
// to implement a server application with TCP

#include <iostream>
#include <boost/asio.hpp>
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

int main()
{
	try
	{
		boost::asio::io_context io_context;		// All programs that use
		// asio need to have at least one boost::asio::io_service object

		// A ip::tcp::acceptor object needs to be created to listen for new
		// connections. It is initialised to listen on TCP port 13, for IPv4
		tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 13));

		for (;;)
		{
			tcp::socket socket(io_context);
			// This is an iterative server, which means that it will handle
			// one connection at a time. Create a socket that will represent
			// the connection to the client, and then wait for a connection
			acceptor.accept(socket);

			// A client is accessing our service.

			// Determine the current time
			std::string message = make_daytime_string();

			// Transfer this information to the clien
			boost::system::error_code ignored_error;
			boost::asio::write(socket, boost::asio::buffer(message), ignored_error);
		}
	}
	catch (std::exception& e)
	{
		// Finally, handle any exceptions
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
