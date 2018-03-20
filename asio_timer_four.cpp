// asio_timer_four.cpp: Using a member function as a handler
//
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

// Instead of defining a free function print as the callback handler, as we
// did in the earlier tutorial programs, we now define a class called printer

class printer
{
	boost::asio::deadline_timer timer_;
	int count_;

public:
	// The constructor of this class will take a reference to the io_service
	// object and use it when initialising the timer_ member. The counter
	// used to shut down the program is now also a member of the class. 
	printer(boost::asio::io_context& io)
		: timer_(io, boost::posix_time::seconds(1)), count_(0)
	{
		// The boost::bind() function works just as well with class member functions
		// as with free functions. Since all non-static class member functions have
		// an implicit this parameter, we need to bind this to the function
		timer_.async_wait(boost::bind(&printer::print, this));
	}

	~printer() // We will print out the final value of the counter
	{	std::cout << "Final count is " << count_ << std::endl;	}

	void print()
	{
		if (count_ < 5)
		{
			std::cout << count_ << std::endl;
			++count_;

			timer_.expires_at(timer_.expires_at() + boost::posix_time::seconds(1));
			timer_.async_wait(boost::bind(&printer::print, this));
		}
	}
};

int main()
{
	boost::asio::io_context io;
	printer p(io);
	io.run();

	return 0;
}
