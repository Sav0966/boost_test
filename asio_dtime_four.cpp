// asio_dtime_four.cpp: A synchronous UDP daytime client
//
// This tutorial program shows how to use asio to implement
// a client application with UDP

#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::udp;

int main(int argc, char* argv[])
{
	char* serv = "echo.u-blox.com"; // use it as default

	try
	{
		if (argc != 2) {
			// The purpose of this application is to access a daytime
			// service, so we need the user to specify the server
			std::cerr << "Usage: server <host>" << std::endl;
			std::cerr << "echo.u-blox.com is used as default" << std::endl;
		}
		else serv = argv[1];

		boost::asio::io_context io_context;		// All programs that use
		// asio need to have at least one boost::asio::io_service object

		udp::resolver resolver(io_context); // We use an ip::udp::resolver
		// object to find the correct remote endpoint to use based on the
		// host and service names. The query is restricted to return only
		// IPv4 endpoints by the ip::udp::v4() argument

		// The ip::udp::resolver::resolve() function is guaranteed to return
		// at least one endpoint in the list if it does not fail. This means
		// it is safe to dereference the return value directly
		udp::endpoint receiver_endpoint =
			*resolver.resolve(udp::v4(), serv, "daytime").begin();

		// Since UDP is datagram-oriented, we won't be using a stream socket
		udp::socket socket(io_context);		// Create an ip::udp::socket and
		socket.open(udp::v4()); // initiate contact with the remote endpoint

		boost::array<char, 1> send_buf = { { 0 } };  // initiate contact
		socket.send_to(boost::asio::buffer(send_buf), receiver_endpoint);

		udp::endpoint sender_endpoint;
		boost::array<char, 128> recv_buf;
		// Now we need to be ready to accept whatever the server sends back
		// to us. The endpoint on our side that receives the server's response
		// will be initialised by ip::udp::socket::receive_from()
		size_t len = socket.receive_from(
			boost::asio::buffer(recv_buf), sender_endpoint);

		std::cout.write(recv_buf.data(), len);
	}
	catch (std::exception& e)
	{
		// Handle any exceptions that may have been thrown
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
