#pragma once
/**
 * Source Reference: https://gist.github.com/yoggy/3323808
 *
 * Modified by FLUX Inc. for Qt framework
 *
 */

#include <boost/asio.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include <string>

#include <QObject>

typedef std::unique_ptr<boost::asio::serial_port> serial_port_ptr;
typedef std::unique_ptr<boost::asio::io_context> io_context_ptr;


#define SERIAL_PORT_READ_BUF_SIZE 256

class SerialPort : public QObject
{
    Q_OBJECT
protected:
    io_context_ptr io_context_;
    serial_port_ptr port_;

    boost::mutex mutex_;
    char read_buf_raw_[SERIAL_PORT_READ_BUF_SIZE];
    std::string read_buf_str_;
    char end_of_line_char_;

private:
    SerialPort();

public:
    static SerialPort& getInstance()
    {
      static SerialPort serial_port_;
      return serial_port_;
    }
    SerialPort(SerialPort const&)     = delete;
    void operator=(SerialPort const&) = delete;
    virtual ~SerialPort(void);

    char end_of_line_char() const;
    void end_of_line_char(const char &c);

    virtual bool start(const char *com_port_name, int baud_rate=9600);
    virtual void stop();
    bool isConnected();

    int write_some(const std::string &buf);
    int write_some(const char *buf, const int &size);

    void clear_buf() { read_buf_str_.clear(); }

signals:
    void connected();
    void responseReceived(QString resp);
    void disconnected();

protected:
    virtual void async_read_some_();
    virtual void on_receive_(const boost::system::error_code& ec, size_t bytes_transferred);
};
