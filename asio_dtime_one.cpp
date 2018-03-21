// asio_dtime_one.cpp: A synchronous TCP daytime client
//
// This tutorial program shows how to use asio to implement
// a client application with TCP

#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

int main(int argc, char* argv[])
{
	char* serv = "time.nist.gov"; // use it as default

	try
	{
		if (argc != 2) {
			// The purpose of this application is to access a daytime
			// service, so we need the user to specify the server
			std::cerr << "Usage: server <host>" << std::endl;
			std::cerr << "time.nist.gov is used as default" << std::endl;
		}
		else serv = argv[1];

		boost::asio::io_context io_context;		// All programs that use
		// asio need to have at least one boost::asio::io_service object

		tcp::resolver resolver(io_context);
		// We need to turn the server name to into a TCP endpoint.
		// To do this we use an ip::tcp::resolver object. The list of
		// endpoints is returned using an iterator of type ip::tcp::resolver::iterator
		tcp::resolver::results_type endpoints = resolver.resolve(serv, "daytime");

		tcp::socket socket(io_context); // Now we create and connect the socket
		boost::asio::connect(socket, endpoints); //The list of endpoints obtained above
		// may contain both IPv4 and IPv6 endpoints, so we need to try each of them until
		// we find one that works. This keeps the client program independent of a specific
		// IP version. The boost::asio::connect() function does this for us automatically

		// The connection is open.All we need to do now
		// is read the response from the daytime service

		for (;;)
		{
			boost::array<char, 128> buf;
			// Instead of a boost::array, we could
			// have used a char [] or std::vector
			boost::system::error_code error;

			// We use a boost::array to hold the received data.
			// The boost::asio::buffer() function automatically determines
			// the size of the array to help prevent buffer overruns.
			size_t len = socket.read_some(boost::asio::buffer(buf), error);

			// When the server closes the connection,
			// the ip::tcp::socket::read_some() function will exit with the
			// boost::asio::error::eof error, which is how we know to exit the loop
			if (error == boost::asio::error::eof) break; // Connection closed cleanly by peer
			else if (error)	throw boost::system::system_error(error); // Some other error

			std::cout.write(buf.data(), len);
		}
	}
	catch (std::exception& e)
	{
		// Handle any exceptions that may have been thrown
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
