#include "serial-port.h"
#include <QDebug>

SerialPort::SerialPort() : end_of_line_char_('\n')
{
}

SerialPort::~SerialPort(void)
{
  stop();
}

char SerialPort::end_of_line_char() const
{
  return this->end_of_line_char_;
}

void SerialPort::end_of_line_char(const char &c)
{
  this->end_of_line_char_ = c;
}

/**
 * @brief Connect to the serial device
 * @param com_port_name /dev/ttyXXX for Linux/MacOSX
 *                      COMx for Windows
 * @param baud_rate
 * @return
 */
bool SerialPort::start(const char *com_port_name, int baud_rate)
{
  boost::system::error_code ec;

  if (!port_) {
    io_context_ = std::make_unique<boost::asio::io_context>();
    port_ = std::make_unique<boost::asio::serial_port>(*io_context_);
  }
  if (port_->is_open()) {
    qInfo() << "error : port is already opened...";
    return false;
  }
  qInfo() << "SerialPort start connection";
  port_->open(com_port_name, ec);
  if (ec) {
    qInfo() << "error : port_->open() failed...com_port_name="
              << com_port_name << ", e=" << ec.message().c_str();
    return false;
  }

  // option settings...
  try {
    port_->set_option(boost::asio::serial_port_base::baud_rate(baud_rate));
    port_->set_option(boost::asio::serial_port_base::character_size(8));
    port_->set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
    port_->set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
    port_->set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));
  } catch (std::exception const &e) {
    qWarning() << "Error connection: " << e.what();
  }
  async_read_some_();

  // NOTE: The following line is necessary for async_read_some to work
  boost::thread t(boost::bind(&boost::asio::io_context::run, io_context_.get()));

  emit connected();

  return true;
}

/**
 * @brief Disconnect from the serial device
 */
void SerialPort::stop()
{
  boost::mutex::scoped_lock look(mutex_);
  bool should_disconnect = false;

  if (port_ && port_->is_open()) {
    port_->cancel();
    port_->close();
    port_.reset(); // clear the shared_ptr (port_ becomes nullptr here)
    qInfo() << "SerialPort disconnect";
    should_disconnect = true;
  }
  if (io_context_) {
    io_context_->stop();
    io_context_.reset();
  }

  clear_buf();
  if (should_disconnect) {
    emit disconnected();
  }
}

bool SerialPort::isConnected() {
  if (port_ && port_->is_open()) {
    return true;
  }
  return false;
}

int SerialPort::write_some(const std::string &buf)
{
  return write_some(buf.c_str(), buf.size());
}

int SerialPort::write_some(const char *buf, const int &size)
{
  //qInfo() << "SerialPort write" << buf;
  boost::system::error_code ec;

  if (!port_ || !port_->is_open()) return -1;
  if (size == 0) return 0;

  return port_->write_some(boost::asio::buffer(buf, size), ec);
}

void SerialPort::async_read_some_()
{
  if (!port_ || !port_->is_open() || !io_context_) return;

  port_->async_read_some(
          boost::asio::buffer(read_buf_raw_, SERIAL_PORT_READ_BUF_SIZE),
          boost::bind(
                  &SerialPort::on_receive_,
                  this, boost::asio::placeholders::error,
                  boost::asio::placeholders::bytes_transferred));
}

void SerialPort::on_receive_(const boost::system::error_code& ec, size_t bytes_transferred)
{
  boost::mutex::scoped_lock look(mutex_);

  if (!port_ || !port_->is_open()) return;
  if (ec) {
    async_read_some_();
    return;
  }

  for (unsigned int i = 0; i < bytes_transferred; ++i) {
    char c = read_buf_raw_[i];
    if (c == end_of_line_char_) {
      emit responseReceived(QString::fromStdString(read_buf_str_));
      read_buf_str_.clear();
    }
    else {
      read_buf_str_ += c;
    }
  }

  async_read_some_(); // start the next read
}
