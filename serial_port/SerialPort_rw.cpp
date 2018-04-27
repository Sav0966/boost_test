#include "Executor.h"
#include "SerialPort.h"
#include <boost/thread.hpp>
#include <boost/date_time.hpp>
#include <boost/program_options.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>

boost::mutex cout_lock;
void Log(const std::string &msg) {
	boost::mutex::scoped_lock lock(cout_lock);
	std::cout << "[" << boost::this_thread::get_id() << "] " << msg << std::endl;
}

typedef boost::tuple<
	boost::posix_time::time_duration::tick_type,
	std::vector<unsigned char>> WriteBufferElement;

class SerialReader : private boost::noncopyable,
	public boost::enable_shared_from_this<SerialReader>
{
	boost::shared_ptr<SerialPort> serialPort_;

	std::string portName_;
	unsigned int baudRate_;
	const boost::scoped_ptr<boost::archive::text_oarchive> &oa_;

	boost::posix_time::ptime lastRead_;

	std::vector<WriteBufferElement> writeBuffer_;

	void OnRead(boost::asio::io_context &ioc, const std::vector<unsigned char> &buffer, size_t bytesRead);
	
public:
	SerialReader(const std::string &portName, int baudRate,
		const boost::scoped_ptr<boost::archive::text_oarchive> &oa,
		const std::vector<WriteBufferElement> &writeBuffer) : portName_(portName),
		baudRate_(baudRate), oa_(oa), writeBuffer_(writeBuffer) {}

	void Create(boost::asio::io_context &ioc)
	{
		try
		{
			serialPort_.reset(new SerialPort(ioc,  portName_));  
			serialPort_->Open(boost::bind(&SerialReader::OnRead, shared_from_this(), _1, _2, _3), baudRate_);

			ioc.post([=, &ioc] {
				uint64_t startTime = 0;

				std::for_each(writeBuffer_.begin(), writeBuffer_.end(),
					[&](const WriteBufferElement &e) {
					startTime += e.get<0>();
					const boost::shared_ptr<boost::asio::deadline_timer>
						timer(new boost::asio::deadline_timer(ioc));

					timer->expires_from_now(boost::posix_time::milliseconds(startTime));
					timer->async_wait([=](const boost::system::error_code &ec) {
						boost::shared_ptr<boost::asio::deadline_timer> t(timer);
						// keep the timer object alive by shared_ptr
						serialPort_->Write(e.get<1>());	}
					); // async_wait([..])
				}); // for_each(.., [&])
			}); // post([..])
		}
		catch (const std::exception &e)
		{
			std::cout << e.what() << std::endl;		
		}
	}
};

void SerialReader::OnRead(boost::asio::io_context &, const std::vector<unsigned char> &buffer, size_t bytesRead)
{
	const boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();

	if (lastRead_ == boost::posix_time::not_a_date_time) lastRead_ = now;

	const std::vector<unsigned char> v(buffer.begin(), buffer.begin()+bytesRead);

	if (oa_) { 		
		const uint64_t offset = (now-lastRead_).total_milliseconds();
		*oa_ << offset << v;
	}

	lastRead_ = now;

	std::copy(v.begin(), v.end(), std::ostream_iterator<unsigned char>(std::cout, ""));
}

int main(int argc, char *argv[])
{
	try
	{
		std::string portName, file;
		int baudRate;
		boost::program_options::options_description desc("Options");

		desc.add_options()
			("help,h", "help")
			("port,p", boost::program_options::value<std::string>(&portName)->required(), "port name (required)")
			("baud,b", boost::program_options::value<int>(&baudRate)->required(), "baud rate (required)")
			("file,f", boost::program_options::value<std::string>(&file), "file to save to");

		boost::program_options::variables_map vm;
		boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);

		if (vm.empty() || vm.count("help"))
		{
			std::cout << desc << "\n";

			portName = "\\\\.\\COM1";
			baudRate = 9600; // TODO: Разобраться со значениями по умолчанию
			// return -1;
		}
		else boost::program_options::notify(vm);
		// don't call notify() until ready to process errors so help alone doesn't cause an error on missing required parameters
		// http://stackoverflow.com/questions/5395503/required-and-optional-arguments-using-boost-library-program-options

		const boost::scoped_ptr<std::ostream> out(file.empty() ? 0 : new std::ofstream(file.c_str()));
		const boost::scoped_ptr<boost::archive::text_oarchive> archive(file.empty() ? 0 : new boost::archive::text_oarchive(*out));

		Executor e;
		e.OnWorkerThreadError = [](boost::asio::io_context &, boost::system::error_code ec) { Log(std::string("Error (asio): ") + boost::lexical_cast<std::string>(ec)); };
		e.OnWorkerThreadException = [](boost::asio::io_context &, const std::exception &ex) { Log(std::string("Exception (asio): ") + ex.what()); };

		e.OnWorkerThreadStart = [](boost::asio::io_context &) { Log(std::string("Start new thread (executor)")); };
		e.OnWorkerThreadStop = [](boost::asio::io_context &) { Log(std::string("Stop the thread (executor)")); };

		std::vector<WriteBufferElement> writeBuffer;
		std::ifstream in("gps_2013-01-15_0106");
		boost::archive::text_iarchive ia(in);

		while (true)
		{
			try
			{
				WriteBufferElement e;
				uint64_t us;
				ia >> us >> e.get<1>();
				e.get<0>() = us;
				writeBuffer.push_back(e);
			}
			catch (const std::exception &)
			{
				break; // end of archive - ignore exception
			}
		}

		const boost::shared_ptr<SerialReader> sp(new SerialReader(portName, baudRate, archive, writeBuffer));
		// for shared_from_this() to work inside of Reader, Reader must already be managed by a smart pointer
		e.OnRun = boost::bind(&SerialReader::Create, sp, _1);

		// e.AddCtrlCHandling(); // TODO: Полная херня !!! Потоки сразу завершаются + исключение
		e.Run(); // TODO: Ctrl-break causes an exception
	}
	catch (const std::exception &e)
	{
		std::cout << "Unknown exception (main): " << e.what() << std::endl;
		return -1;
	}

	return 0;
}
