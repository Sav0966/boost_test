// asio_timer_three.cpp: Binding arguments to a handler
//
#include <iostream>
#include <boost/bind.hpp>
// Since this example uses timers, we need to include the
// appropriate Boost.Date_Time header file for manipulating times.
#include <boost/date_time/posix_time/posix_time.hpp> // seconds()
#include <boost/asio.hpp> // needs a definition of targeted system

// To implement a repeating timer using asio you need to change
// the timer's expiry time in your callback function, and to then
// start a new asynchronous wait. You will observe that there is
// no explicit call to ask the io_service to stop. Otherwise, by not
// starting a new asynchronous wait on the timer when count reaches
// 5, the io_service will run out of work and stop running.

void print(const boost::system::error_code& /*e*/,
	boost::asio::deadline_timer* t, int* count)
{
	if (*count < 5)
	{
		std::cout << *count << std::endl;
		++(*count);

		// By calculating the new expiry time relative to the old,
		// we can ensure that the timer does not drift away from the
		// hole-second mark due to any delays in processing the handler
		t->expires_at(t->expires_at() + boost::posix_time::seconds(1));

		// The deadline_timer::async_wait() function expects
		// a handler function (or function object) with the signature
		// void(const boost::system::error_code&). Binding the additional
		// parameters converts your print function into a function object
		// that matches the signature correctly. See the Boost.Bind ...
		t->async_wait(boost::bind(print,
			boost::asio::placeholders::error, t, count));
	}
}

int main()
{
	boost::asio::io_context io; // All programs that use asio
	// need to have at least one boost::asio::io_service object

	int count = 0;
	// The core asio classes that provide I/O functionality (or as in
	// this case timer functionality) always take a reference to an
	// io_service as their first constructor argument.The second argument
	// to the constructor sets the timer to expire 1 second from now.
	boost::asio::deadline_timer t(io, boost::posix_time::seconds(1));
	t.async_wait(boost::bind(print, // See the Boost.Bind ...
		boost::asio::placeholders::error, &t, &count));

	// It is important to remember to give the io_context some work to do
	// before calling io_service::run(). If the io_context would not have
	// had any work to do, io_service::run() would have returned immediately
	io.run(); // It will stop running in six secconds

	std::cout << "Final count is " << count << std::endl;

	return 0;
}
