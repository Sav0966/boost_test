// asio_timer_one.cpp: Using a timer synchronously and asynchronously
//
// This tutorial program introduces asio by showing how to perform a
// blocking wait on a timer and asio's asynchronous callback functionality

#include <iostream>

// All of the asio classes can be used by simply
// including the "asio.hpp" header file.
#include <boost/asio.hpp>

// Since this example uses timers, we need to include the
// appropriate Boost.Date_Time header file for manipulating times.
#include <boost/date_time/posix_time/posix_time.hpp> // seconds()

// Using asio's asynchronous functionality means having a callback
// function that will be called when an asynchronous operation completes.
void print(const boost::system::error_code& /*e*/)
{
	std::cout << "Wait 5 seconds asynchronously." << std::endl;
}

int main()
{
	boost::asio::io_context io; // All programs that use asio
	// need to have at least one boost::asio::io_service object

	std::cout << "Hello, world!" << std::endl; // Start

	// The core asio classes that provide I/O functionality (or as in
	// this case timer functionality) always take a reference to an
	// io_service as their first constructor argument.The second argument
	// to the constructor sets the timer to expire 5 seconds from now.
	boost::asio::deadline_timer ta(io, boost::posix_time::seconds(5));

	// Using a timer synchronously
	boost::asio::deadline_timer ts(io, boost::posix_time::seconds(3));
	ts.wait(); // will not return until the timer has expired
	std::cout << "Wait 3 seconds synchronously." << std::endl;

	ta.async_wait(&print); // Using a timer asynchronously

	// It is important to remember to give the io_context some workto do
	// before calling io_service::run(). If the io_context would not have
	// had any work to do, io_service::run() would have returned immediately
	io.run(); // Finally, we must call the io_service::run()

	return 0;
}
