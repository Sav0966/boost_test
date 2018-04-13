#include "Executor.h"
#include <boost/thread.hpp>

void Executor::WorkerThread(boost::asio::io_context &ioc) {
	if (OnWorkerThreadStart)
		OnWorkerThreadStart(ioc);

	while (true) {
		try
		{
			boost::system::error_code ec;
			// Passing an ec object to run() causes Asio
			// to return its own errors via the code ...
			ioc.run(ec);

			if (ec && OnWorkerThreadError)
				OnWorkerThreadError(ioc, ec);

			break; // run() is not called again
			//  when an Asio error is generated

		}
		catch (const std::exception &ex) {
			// ... but work that is serviced through
			// Asio::run may throw its own exceptions

			if (OnWorkerThreadException)
				OnWorkerThreadException(ioc, ex);

			// run() is called again if an exception is raised by a work
			// item so processing can continue for additional work items
		}
	}

	if (OnWorkerThreadStop)
		OnWorkerThreadStop(ioc);
}

void Executor::AddCtrlCHandling()
{ // stop when ctrl-c is pressed
	boost::asio::signal_set sig_set(io_, SIGTERM, SIGINT);
	sig_set.async_wait(boost::bind(&boost::asio::io_context::stop, boost::ref(io_)));
}

void Executor::Run(unsigned int numThreads)
{
	if (OnRun)
		OnRun(io_);

	boost::thread_group workerThreads;

	// Create a thread pool
	for (unsigned int i = 0;
		i < ((numThreads == (unsigned int)-1) ? // -1 => the number of physical execution units
		(boost::thread::hardware_concurrency()) : numThreads); ++i) // (number of CPUs or cores)
		workerThreads.create_thread(boost::bind(&Executor::WorkerThread, this, boost::ref(io_)));

	workerThreads.join_all(); // Waiting for terminations of all threads
}
