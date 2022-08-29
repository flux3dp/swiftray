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
#include <memory>
#include <string>

#include <QObject>

#ifdef Q_OS_MACOS
typedef std::unique_ptr<boost::asio::serial_port> serial_port_ptr;
typedef std::unique_ptr<boost::asio::io_context> io_context_ptr;
#define SERIAL_PORT_READ_BUF_SIZE 256
#endif
#ifdef Q_OS_WIN
class SerialPortImpl;
#endif

class SerialPort : public QObject
{
    Q_OBJECT
public:

    /**
     * Default constructor
     */
    SerialPort();
    /**
     * Constructor. Opens a serial port
     * Format is 8N1, flow control is disabled.
     * @param devname port name gotten from QSerialPortInfo.portName() (e.g. COM3, tty.1231231)
     * @param baudrate port baud rate, example 115200
     */
    SerialPort(QString devname, unsigned int baudrate);

    /**
     * Destructor
     */
    ~SerialPort();

    void operator=(SerialPort const&) = delete;

    /**
     * Opens a serial port
     * @param devname port name gotten from QSerialPortInfo.portName() (e.g. COM3, tty.1231231)
     * @param baudrate port baud rate, example 115200
     * Format is 8N1, flow control is disabled.
     */
    bool open(QString devname, unsigned int baudrate);

    /**
     * Closes a serial port.
     */
    void close();

    /**
     * @return true if the port is open
     */
    bool isOpen() const;

    int write(const QString data) const;
    int write(const std::string buf) const;
    int write(const char *buf, const int &size) const;

    /**
     * @return true if any error
     */
    bool errorStatus() const;

    /**
     * @return port name (get from QSerialPortInfo) if opened, otherwise, empty string
     */
     QString portName() const;

signals:
    void connected();
    void disconnected();
    void lineReceived(QString resp);

private:
    QString port_name_;
#ifdef Q_OS_MACOS
    boost::mutex mutex_;
    io_context_ptr io_context_;
    serial_port_ptr serial_;
    char read_buf_raw_[SERIAL_PORT_READ_BUF_SIZE];
    std::string read_buf_str_;
    void asyncReadSome();
    void onReceive(const boost::system::error_code& ec, size_t bytes_transferred);
#endif
#ifdef Q_OS_WIN
    /**
     * Called when data is received
     */
    void readCallback(const char *data, size_t size);
    std::shared_ptr<SerialPortImpl> pimpl_; ///< Pimpl idiom
#endif
};
