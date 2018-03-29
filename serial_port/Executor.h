#ifndef __EXECUTOR_H__
#define __EXECUTOR_H__

#include <boost/asio.hpp>
#include <boost/function.hpp>

class Executor : private boost::noncopyable {
	protected:
		boost::asio::io_context io_;
		void WorkerThread(boost::asio::io_context &io);

	public:
		boost::function<void(boost::asio::io_context &)> OnRun;
		boost::function<void(boost::asio::io_context &)> OnWorkerThreadStart;
		boost::function<void(boost::asio::io_context &, boost::system::error_code)> OnWorkerThreadError;
		boost::function<void(boost::asio::io_context &, const std::exception &)> OnWorkerThreadException;
		boost::function<void(boost::asio::io_context &)> OnWorkerThreadStop;

		void AddCtrlCHandling();
		boost::asio::io_context & GetIOContext() { return io_; }

		void Run(unsigned int numThreads = -1);
	};

#endif
