// asio_dtime_five.cpp: A synchronous UDP daytime server
//
// This tutorial program shows how to use asio
// to implement a server application with UDP

#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using boost::asio::ip::udp;

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

		// Create an ip::udp::socket object to receive requests on UDP port 13
		udp::socket socket(io_context, udp::endpoint(udp::v4(), 13));

		for (;;)
		{
			boost::array<char, 1> recv_buf;
			udp::endpoint remote_endpoint;
			boost::system::error_code error;

			// Wait for a client to initiate contact with us.
			// The remote_endpoint object will be populated
			// by ip::udp::socket::receive_from()
			socket.receive_from(boost::asio::buffer(recv_buf),
				remote_endpoint, 0, error);

			if (error && error != boost::asio::error::message_size)
				throw boost::system::system_error(error);

			// Determine what we are going to send back to the client
			std::string message = make_daytime_string();

			// Send the response to the remote_endpoint
			boost::system::error_code ignored_error;
			socket.send_to(boost::asio::buffer(message),
				remote_endpoint, 0, ignored_error);
		}
	}
	catch (std::exception& e)
	{
		// Finally, handle any exceptions
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
