#ifndef __SERIALPORT_H__
#define __SERIALPORT_H__

#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>

class SerialPort : private boost::noncopyable,
	public boost::enable_shared_from_this<SerialPort>
{
	public:
		SerialPort(boost::asio::io_context &ioc, const std::string &portName);
		~SerialPort();

		typedef boost::asio::serial_port_base::parity parity;
		typedef boost::asio::serial_port_base::stop_bits stop_bits;
		typedef boost::asio::serial_port_base::flow_control flow_control;
		typedef boost::asio::serial_port_base::character_size character_size;

		typedef boost::function<void(boost::asio::io_context &,
			const std::vector<unsigned char> &, size_t)> onread_handler;

		void Open(const onread_handler &onRead, unsigned int baudRate, // def = 8N1 without control
			parity par = parity(parity::none), flow_control flow = flow_control(flow_control::none),
			character_size siz = character_size(8U), stop_bits bits = stop_bits(stop_bits::one));

		void Close();

		void Write(const unsigned char *buffer, size_t bufferLength);
		void Write(const std::vector<unsigned char> &buffer);
		void Write(const std::string &buffer);

	private:
		// Clear all characters pending on the serial port
		boost::system::error_code Flush();

		void WriteBegin();
		void WriteComplete(const boost::system::error_code &ec);
		void ReadComplete(const boost::system::error_code &ec, size_t bytesTransferred);
		void ReadBegin();

		void SetErrorCode(const boost::system::error_code &ec); // Locked by mutex

		boost::asio::serial_port serialPort_;

		boost::mutex writeQueueMutex_, writeBufferMutex_;
		std::vector<unsigned char> writeQueue_, writeBuffer_;

		std::vector<unsigned char> readBuffer_;
		onread_handler onRead_;

		boost::mutex errorCodeMutex_;
		boost::system::error_code errorCode_;

		bool isOpen_;
};

#endif
