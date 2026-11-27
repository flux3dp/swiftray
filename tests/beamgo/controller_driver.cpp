#include <beamgo/controller.h>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QPointer>
#include <QSettings>
#include <QTcpServer>
#include <QTcpSocket>
#include <iostream>

// Test-only JSON-lines bridge to a supplied pseudo-terminal. Never scans real hardware.
int main(int argc, char** argv) {
  QCoreApplication app(argc, argv);
  QCoreApplication::setOrganizationName("BeamGoTests");
  QCoreApplication::setApplicationName("ControllerTests");
  if (argc != 3) return 2;
  QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, argv[2]);
  // macOS PTYs cannot implement IOSSIOSPEED for 921600. No physical baud rate exists on a PTY.
  beamgo::Controller controller(argv[1], nullptr, 115200);
  QTcpServer server;
  QObject::connect(&server, &QTcpServer::newConnection, &app, [&] {
    auto* socket = server.nextPendingConnection();
    auto buffer = std::make_shared<QByteArray>();
    QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
    QObject::connect(socket, &QTcpSocket::readyRead, &app, [&, socket, buffer] {
      *buffer += socket->readAll();
      while (buffer->contains('\n')) {
        const int end = buffer->indexOf('\n');
        const auto request = QJsonDocument::fromJson(buffer->left(end)).object();
        buffer->remove(0, end + 1);
        QPointer<QTcpSocket> guarded = socket;
        controller.action(request["action"].toString(), request["params"].toObject(),
            [guarded, id = request["id"]](QJsonObject result, QString type) {
          if (guarded) guarded->write(QJsonDocument(QJsonObject{{"id", id}, {"type", type}, {"result", result}}).toJson(QJsonDocument::Compact) + '\n');
        });
      }
    });
  });
  QObject::connect(&controller, &beamgo::Controller::discovered, &app, [&](QJsonObject) {
    if (!server.listen(QHostAddress::LocalHost)) app.exit(3);
    std::cout << server.serverPort() << std::endl;
  });
  QTimer::singleShot(0, &controller, &beamgo::Controller::initialize);
  return app.exec();
}
