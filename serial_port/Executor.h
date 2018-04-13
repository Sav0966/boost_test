#ifndef __EXECUTOR_H__
#define __EXECUTOR_H__

#include <boost/asio.hpp>
#include <boost/function.hpp>

// Establish a thread pool to call the run() method of
// io_context via the Executor::WorkerThread() method,
// therefore the copy operation is not appropriate

class Executor : private boost::noncopyable
{
	protected:
		boost::asio::io_context io_;
		void WorkerThread(boost::asio::io_context &ioc);

	public:
		// Callback functions are provided which will be
		// invoked at interesting points of the execution
		boost::function<void(boost::asio::io_context &)> OnRun;
		boost::function<void(boost::asio::io_context &)> OnWorkerThreadStart;
		boost::function<void(boost::asio::io_context &, boost::system::error_code)> OnWorkerThreadError;
		boost::function<void(boost::asio::io_context &, const std::exception &)> OnWorkerThreadException;
		boost::function<void(boost::asio::io_context &)> OnWorkerThreadStop;

		boost::asio::io_context & GetIOContext() { return io_; }

		void AddCtrlCHandling();  // Intercept the pressing of Ctrl-C
		void Run(unsigned int numThreads = -1); // Start the Executor
};

#endif
