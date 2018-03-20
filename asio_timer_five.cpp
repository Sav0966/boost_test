// asio_timer_five.cpp: Synchronising handlers in multithreaded programs
//
// This tutorial demonstrates the use of the boost::asio::io_service::strand
// class to synchronise callback handlers in a multithreaded program
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>

// We start by defining a class called printer, similar to the class
// in the previous tutorial. This class will extend the previous
// tutorial by running two timers in parallel

class printer
{
	boost::asio::io_context::strand strand_; // An boost::asio::io_service::strand
	// is an executor that guarantees that, for those handlers that are dispatched
	// through it, an executing handler will be allowed to complete before the next
	// one is started. This is guaranteed irrespective of the number of threads that
	// are calling io_service::run(). Of course, the handlers may still execute
	// concurrently with other handlers that were not dispatched through an
	// boost::asio::io_service::strand, or were dispatched through a different
	// boost::asio::io_service::strand object
	boost::asio::deadline_timer timer1_;
	boost::asio::deadline_timer timer2_;
	int count_;

public:
	printer(boost::asio::io_context& io)
		: strand_(io), // In addition to initialising a pair of
		// boost::asio::deadline_timer members, the constructor
		// initialises the strand_ data member
		timer1_(io, boost::posix_time::seconds(1)),
		timer2_(io, boost::posix_time::seconds(1)),
		count_(0)
	{
		// When initiating the asynchronous operations, each callback
		// handler is "bound" to an boost::asio::io_service::strand object.
		// The boost::asio::io_service::strand::bind_executor() function returns
		// a new handler that automatically dispatches its contained handler
		// through the boost::asio::io_service::strand object. By binding
		// the handlers to the same boost::asio::io_service::strand,
		// we are ensuring that they cannot execute concurrently
		timer1_.async_wait(boost::asio::bind_executor(strand_,
			boost::bind(&printer::print1, this)));

		timer2_.async_wait(boost::asio::bind_executor(strand_,
			boost::bind(&printer::print2, this)));
	}

	~printer() // We will print out the final value of the counter
	{	std::cout << "Final count is " << count_ << std::endl;	}

	// In a multithreaded program, the handlers for asynchronous
	// operations should be synchronised if they access shared resources.
	// In this tutorial, the shared resources used by the handlers
	// (print1 and print2) are std::cout and the count_ data member

	void print1()
	{
		if (count_ < 10)
		{
			std::cout << "Timer 1: " << count_ << std::endl;
			++count_;

			timer1_.expires_at(timer1_.expires_at() + boost::posix_time::seconds(1));

			timer1_.async_wait(boost::asio::bind_executor(strand_,
				boost::bind(&printer::print1, this)));
		}
	}

	void print2()
	{
		if (count_ < 10)
		{
			std::cout << "Timer 2: " << count_ << std::endl;
			++count_;

			timer2_.expires_at(timer2_.expires_at() + boost::posix_time::seconds(1));

			timer2_.async_wait(boost::asio::bind_executor(strand_,
				boost::bind(&printer::print2, this)));
		}
	}
};

// The main function now causes io_service::run() to be called
// from two threads : the main thread and one additional thread.
// This is accomplished using an boost::thread object
//
// Just as it would with a call from a single thread, concurrent
// calls to io_service::run() will continue to execute while there
// is "work" left to do. The background thread will not exit
// until all asynchronous operations have completed

int main()
{
	boost::asio::io_context io;
	printer p(io);

	boost::thread t(boost::bind(&boost::asio::io_context::run, &io));

	io.run();
	t.join();

	return 0;
}
