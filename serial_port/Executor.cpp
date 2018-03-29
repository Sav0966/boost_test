#include "Executor.h"
#include <boost/thread.hpp>

void Executor::WorkerThread(boost::asio::io_context &io) {
	if (OnWorkerThreadStart)
		OnWorkerThreadStart(io);

	while (true) {
		try
		{
			boost::system::error_code ec;
			io.run(ec);

			if (ec && OnWorkerThreadError)
				OnWorkerThreadError(io, ec);

			break;
		}
		catch (const std::exception &ex) {
			if (OnWorkerThreadException)
				OnWorkerThreadException(io, ex);
		}
	}

	if (OnWorkerThreadStop)
		OnWorkerThreadStop(io);
}

void Executor::AddCtrlCHandling() {
	// stop when ctrl-c is pressed
	boost::asio::signal_set sig_set(io_, SIGTERM, SIGINT);
	sig_set.async_wait(boost::bind(&boost::asio::io_context::stop, boost::ref(io_)));
}

void Executor::Run(unsigned int numThreads) {
	if (OnRun)
		OnRun(io_);

	boost::thread_group workerThreads;
	for (unsigned int i = 0; i < ((numThreads == (unsigned int)-1) ?
									(boost::thread::hardware_concurrency()) : numThreads); ++i)
		workerThreads.create_thread(boost::bind(&Executor::WorkerThread, this, boost::ref(io_)));
	workerThreads.join_all();
}
