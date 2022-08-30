#include "serial-port.h"
#include <connection/QAsyncSerial/AsyncSerial.h>
#include <QDebug>

#ifdef Q_OS_WIN
/**
 * Implementation details of QAsyncSerial class.
 */
class SerialPortImpl {
public:
    CallbackAsyncSerial serial_;
    QString receivedData;
};

SerialPort::SerialPort(): pimpl_(new SerialPortImpl) {

}

SerialPort::SerialPort(QString devname, unsigned int baudrate): pimpl_(new SerialPortImpl) {
  open(devname,baudrate);
}
#endif

#ifdef Q_OS_MACOS
SerialPort::SerialPort() {

}

SerialPort::SerialPort(QString devname, unsigned int baudrate) {
  open(devname, baudrate);
}
#endif

SerialPort::~SerialPort() {
#ifdef Q_OS_MACOS
  close();
#endif
#ifdef Q_OS_WIN
  pimpl_->serial_.clearCallback();
  try {
    pimpl_->serial_.close();
  } catch(...)
  {
    //Don't throw from a destructor
  }
#endif
}


/**
 * @brief Connect to the serial device
 * @param devname /dev/ttyXXX for Linux/MacOSX
 *                      COMx for Windows
 * @param baudrate
 * @return
 */
bool SerialPort::open(QString port_name, unsigned int baudrate) {
  QString full_port_path;
  if (port_name.startsWith("tty")) { // Linux/macOSX
    full_port_path += "/dev/";
    full_port_path += port_name;
  } else { // Windows COMx
    full_port_path = port_name;
  }

#ifdef Q_OS_MACOS
  boost::system::error_code ec;

  if (serial_ && serial_->is_open()) {
    //close();
    qInfo() << "error : port is already opened...";
    return false;
  }
  if (!serial_) {
    io_context_ = std::make_unique<boost::asio::io_context>();
    serial_ = std::make_unique<boost::asio::serial_port>(*io_context_);
  }
  qInfo() << "SerialPort start connection";
  serial_->open(full_port_path.toStdString(), ec);
  if (ec) {
    qInfo() << "error : port_->open() failed...com_port_name="
              << full_port_path << ", e=" << ec.message().c_str();
    return false;
  }

  // option settings...
  try {
    serial_->set_option(boost::asio::serial_port_base::baud_rate(baudrate));
    serial_->set_option(boost::asio::serial_port_base::character_size(8));
    serial_->set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
    serial_->set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
    serial_->set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));
  } catch (std::exception const &e) {
    qWarning() << "Error connection: " << e.what();
  }
  // Register async read operation
  asyncReadSome();
  // Run event loop in a new thread (to make it non-blocking here)
  boost::thread t([=]() {
    io_context_->run();
  });

#endif
#ifdef Q_OS_WIN
  bool result = false;
  try {
    pimpl_->serial_.open(full_port_path.toStdString(),baudrate);
    result = true;
  } catch(boost::system::system_error&)
  {
    //Errors during open
    return false;
  }
  pimpl_->serial_.setCallback(boost::bind(&SerialPort::readCallback,this, _1, _2));
  if (result != true) {
    return false;
  }
#endif
  port_name_ = port_name;
  emit connected();
  return true;
}

void SerialPort::close() {
  bool should_emit_disconnect = false;
#ifdef Q_OS_MACOS
  boost::mutex::scoped_lock look(mutex_);

  if (serial_ && serial_->is_open()) {
    serial_->cancel();
    serial_->close();
    serial_.reset(); // clear the shared_ptr (port_ becomes nullptr here)
    qInfo() << "SerialPort disconnect";
    should_emit_disconnect = true;
  }

  if (io_context_) {
    io_context_->stop();
    io_context_.reset(nullptr);
  }

  read_buf_str_.clear();
#endif
#ifdef Q_OS_WIN
  pimpl_->serial_.clearCallback();
  if (pimpl_->serial_.isOpen()) {
    should_emit_disconnect = true;
  }
  try {
    pimpl_->serial_.close();
  } catch(boost::system::system_error&)
  {
    //Errors during port close
  }
  pimpl_->receivedData.clear();//Clear eventual data remaining in read buffer
#endif
  if (should_emit_disconnect) {
    emit disconnected();
  }
}

bool SerialPort::isOpen() const {
#ifdef Q_OS_MACOS
  if (serial_ && serial_->is_open()) {
    return true;
  }
  return false;
#endif
#ifdef Q_OS_WIN
  return pimpl_->serial_.isOpen();
#endif
}

int SerialPort::write(const QString data) const {
  return write(data.toStdString().c_str(), data.size());
}

int SerialPort::write(const std::string buf)const {
  return write(buf.c_str(), buf.size());
}

int SerialPort::write(const char *buf, const int &size) const {
#ifdef Q_OS_MACOS
  //qInfo() << "SerialPort write" << buf;
  boost::system::error_code ec;

  if (!serial_ || !serial_->is_open()) return -1;
  if (size == 0) return 0;

  return serial_->write_some(boost::asio::buffer(buf, size), ec);
#endif
#ifdef Q_OS_WIN
  pimpl_->serial_.writeString(std::string(buf));
  return size;
#endif
}

#ifdef Q_OS_MACOS
void SerialPort::asyncReadSome() {
  if (!serial_ || !serial_->is_open() || !io_context_) return;

  serial_->async_read_some(
          boost::asio::buffer(read_buf_raw_, SERIAL_PORT_READ_BUF_SIZE),
          boost::bind(
                  &SerialPort::onReceive,
                  this, boost::asio::placeholders::error,
                  boost::asio::placeholders::bytes_transferred));
}

void SerialPort::onReceive(const boost::system::error_code& ec, size_t bytes_transferred) {
  boost::mutex::scoped_lock look(mutex_);

  if (!serial_ || !serial_->is_open()) return;
  if (ec) {
    asyncReadSome();
    return;
  }

  for (unsigned int i = 0; i < bytes_transferred; ++i) {
    char c = read_buf_raw_[i];
    if (c == '\n') {
      emit lineReceived(QString::fromStdString(read_buf_str_));
      read_buf_str_.clear();
    }
    else {
      read_buf_str_ += c;
    }
  }

  asyncReadSome(); // start the next read
}
#endif
#ifdef Q_OS_WIN
void SerialPort::readCallback(const char *data, size_t size){
  pimpl_->receivedData += QString::fromLatin1(data,size);
  if(pimpl_->receivedData.contains('\n')){
    QStringList lineList = pimpl_->receivedData.split(QRegExp("\r\n|\n"));
    //If line ends with \n lineList will contain a trailing empty string
    //otherwise it will contain part of a line without the terminating \n
    //In both cases lineList.at(lineList.size()-1) should not be sent
    //with emit.
    int numLines = lineList.size()-1;
    pimpl_->receivedData = lineList.at(lineList.size()-1);
    for (int i=0; i < numLines; i++) {
      emit lineReceived(lineList.at(i));
    }
  }
}
#endif

bool SerialPort::errorStatus() const {
#ifdef Q_OS_MACOS
  // TODO: ?
  return false;
#endif
#ifdef Q_OS_WIN
  return pimpl_->serial_.errorStatus();
#endif
}

QString SerialPort::portName() const {
  if (isOpen()) {
    return port_name_;
  } else {
    return QString{};
  }
}
