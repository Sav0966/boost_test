#include "SerialPort.h"
#include <boost/bind.hpp>

// The Flush() method can be useful to clear all characters pending on the serial port,
// especially when communication is first begun or upon an error condition. This does not
// appear to be provided by Asio, but can be implemented, as was inspired by
// http://www.progtown.com/topic90228-how-for-boost-asio-serialport-to-make-flush.html

boost::system::error_code SerialPort::Flush()
{
	boost::system::error_code ec;

#if !defined(BOOST_WINDOWS) && !defined(__CYGWIN__)
	const bool isFlushed = !::tcflush(serialPort_.native_handle(), TCIOFLUSH);
	if (!isFlushed)
		ec = boost::system::error_code(errno, boost::asio::error::get_system_category());
#else
	const bool isFlushed = ::PurgeComm(serialPort_.native_handle(),
		PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);
	if (!isFlushed)
		ec = boost::system::error_code(::GetLastError(), boost::asio::error::get_system_category()); 
#endif 

	return ec;
}

void SerialPort::SetErrorCode(const boost::system::error_code &ec)
{
	if (ec) {
		boost::mutex::scoped_lock lock(errorCodeMutex_);
		errorCode_ = ec;
	}
}

void SerialPort::ReadBegin()
{
	serialPort_.async_read_some(boost::asio::buffer(readBuffer_),
		boost::bind(&SerialPort::ReadComplete, shared_from_this(),
		boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void SerialPort::ReadComplete(const boost::system::error_code &ec, size_t bytesTransferred)
{
	if (!ec) {
		if (onRead_ && (bytesTransferred > 0))
			// callback executes before any additional reads are queued,
			// so access to the buffer is guaranteed as the buffer won't be overwritten
			onRead_(boost::ref(serialPort_.get_io_context()), boost::cref(readBuffer_), bytesTransferred);

		ReadBegin();  // queue another read
	}
	else { Close(); SetErrorCode(ec); }
}

void SerialPort::Write(const unsigned char *buffer, size_t bufferLength)
{
	{ // Obtain a lock on the write queue and copy data
		boost::mutex::scoped_lock lock(writeQueueMutex_);
		writeQueue_.insert(writeQueue_.end(), buffer, buffer + bufferLength);
	}

	// Invoke WriteBegin() asynchronously by posting
	// it to the io_context object to perform the write
	serialPort_.get_io_context().post(
		boost::bind(&SerialPort::WriteBegin, shared_from_this()));

	// The caller of Write() can then continue processing
	// and not block while the write is in progress
}

void SerialPort::WriteBegin()
{
	boost::mutex::scoped_lock writeBufferlock(writeBufferMutex_);
	if (writeBuffer_.size() != 0)
		return;  // a write is in progress, so don't start another

	boost::mutex::scoped_lock writeQueuelock(writeQueueMutex_);
	if (writeQueue_.size() == 0)
		return;  // nothing to write

	// allocate a larger buffer if needed
	const std::vector<unsigned char>::size_type writeQueueSize = writeQueue_.size();
	if (writeQueueSize > writeBuffer_.size()) writeBuffer_.resize(writeQueueSize);
	// resize, not reserve, as copy can't create elements - only copy over existing ones

	// copy the queued bytes to the write buffer, and clear the queued bytes
	std::copy(writeQueue_.begin(), writeQueue_.end(), writeBuffer_.begin());
	writeQueue_.clear();

	boost::asio::async_write(serialPort_, boost::asio::buffer(writeBuffer_, writeQueueSize),
		boost::bind(&SerialPort::WriteComplete, shared_from_this(), boost::asio::placeholders::error));
}

void SerialPort::WriteComplete(const boost::system::error_code &ec)
{
	if (!ec) {
		{
			// everything in the buffer was sent, so set the length to
			// 0 so WriteBegin knows a write is no longer in progress
			boost::mutex::scoped_lock lock(writeBufferMutex_);
			writeBuffer_.clear();
		}

		// more bytes to send may have arrived while the write
		WriteBegin();		// was in progress, so check again
	}
	else { Close(); SetErrorCode(ec); }
}

SerialPort::SerialPort(boost::asio::io_context &ioService, const std::string& portName) : 
	serialPort_(ioService, portName), // TODO: Убрать из констуктора (открывает порт)
	isOpen_(false)
{
	readBuffer_.resize(128); // TODO: Убрать из конструктора
}

void SerialPort::Open(const onread_handler &onRead, unsigned int baudRate,
	parity par, flow_control flow, character_size siz, stop_bits bits)
{
		onRead_ = onRead;
		serialPort_.set_option(boost::asio::serial_port_base::baud_rate(baudRate));
		serialPort_.set_option(siz); serialPort_.set_option(bits);
		serialPort_.set_option(par); serialPort_.set_option(flow);

		const boost::system::error_code ec = Flush();
		if (ec)
			SetErrorCode(ec);

		isOpen_ = true;

		if (onRead_) {
			// don't start the async reader unless a read callback has been provided
			// be sure shared_from_this() is only called after object is already managed
			// in a shared_ptr, so need to fully construct, then call Open() on the ptr
			serialPort_.get_io_context().post(boost::bind(&SerialPort::ReadBegin, shared_from_this()));
			// want read to start from a thread in io_context
		}
}

SerialPort::~SerialPort() { Close(); }

void SerialPort::Close()
{
	if (isOpen_) {
		isOpen_ = false;

		// Outstanding requests are cancelled first,
		// and then the port itself is closed
		boost::system::error_code ec;
		serialPort_.cancel(ec);
		SetErrorCode(ec);

		serialPort_.close(ec);
		SetErrorCode(ec);
	}
}

void SerialPort::Write(const std::vector<unsigned char> &buffer) {
	Write(&buffer[0], buffer.size());
}

void SerialPort::Write(const std::string &buffer) {
	Write(reinterpret_cast<const unsigned char *>(buffer.c_str()), buffer.size());
}
