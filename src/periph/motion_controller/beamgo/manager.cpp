#include "manager.h"

#include <QPointer>
#include <QSerialPortInfo>

namespace beamgo {
namespace {
bool active(const QJsonObject& info) {
  const int status = info["st_id"].toInt();
  return status == 1 || status == 16 || status == 48;
}
}

Manager::Manager(QObject* parent) : QObject(parent) {
  scan_.setInterval(2000);
  connect(&scan_, &QTimer::timeout, this, &Manager::scan);
  scan_.start();
  QTimer::singleShot(0, this, &Manager::scan);
}

Manager::~Manager() {
  scan_.stop();
  for (const auto& session : sessions_) {
    QMetaObject::invokeMethod(session.controller, [controller = session.controller] {
      controller->shutdown();
    }, Qt::BlockingQueuedConnection);
  }
  for (auto* thread : threads_) { thread->quit(); thread->wait(); }
}

QJsonArray Manager::devices() const {
  QJsonArray result;
  for (const auto& session : sessions_) if (!session.info.isEmpty()) result.append(session.info);
  return result;
}

bool Manager::hasActiveJob() const {
  for (const auto& session : sessions_) if (session.starting || active(session.info)) return true;
  return false;
}

void Manager::retire(const QString& port) {
  if (!sessions_.contains(port)) return;
  const auto session = sessions_.take(port);
  QMetaObject::invokeMethod(session.controller, [controller = session.controller] {
    controller->shutdown();
    QThread::currentThread()->quit();
  }, Qt::QueuedConnection);
}

void Manager::scan() {
  QSet<QString> present;
  for (const auto& info : QSerialPortInfo::availablePorts()) {
    if (!info.hasVendorIdentifier() || info.vendorIdentifier() != 0x303a) continue;
    const QString port = info.systemLocation();
    // On macOS prefer the outgoing CDC endpoint and avoid probing its duplicate tty endpoint.
#ifdef Q_OS_MACOS
    if (port.startsWith("/dev/tty.")) continue;
#endif
    present.insert(port);
    if (sessions_.contains(port)) continue;
    auto* thread = new QThread(this);
    auto* controller = new Controller(port);
    controller->moveToThread(thread);
    threads_.insert(thread);
    sessions_.insert(port, {controller, thread, {}, false});
    connect(thread, &QThread::started, controller, &Controller::initialize);
    connect(thread, &QThread::finished, controller, &QObject::deleteLater);
    connect(thread, &QThread::finished, this, [this, thread] { threads_.remove(thread); thread->deleteLater(); });
    const auto update = [this, port, controller](QJsonObject device) {
      if (sessions_.contains(port) && sessions_[port].controller == controller) sessions_[port].info = std::move(device);
    };
    connect(controller, &Controller::discovered, this, update);
    connect(controller, &Controller::statusChanged, this, update);
    connect(controller, &Controller::disconnected, this, [this, port, controller] {
      if (sessions_.contains(port) && sessions_[port].controller == controller) retire(port);
    });
    thread->start();
  }
  for (const auto& port : sessions_.keys()) if (!present.contains(port)) retire(port);
}

void Manager::action(const QString& port, const QString& action, const QJsonObject& params, Reply reply) {
  for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
    if (it->info["port"].toString() != port) continue;
    const bool starts = action == "start" || action == "startFraming";
    if (starts && hasActiveJob()) {
      reply({{"success", false}, {"error", "JOB_RUNNING"}}, "callback");
      return;
    }
    if (starts) it->starting = true;
    const QString physical_port = it.key();
    QPointer<Manager> self = this;
    QMetaObject::invokeMethod(it->controller, [self, controller = it->controller, physical_port, starts,
                                             action, params, reply = std::move(reply)] {
      controller->action(action, params, [self, controller, physical_port, starts, reply](QJsonObject result, QString type) {
        if (!self) return;
        QMetaObject::invokeMethod(self, [self, controller, physical_port, starts, reply, result, type] {
          if (!self) return;
          if (starts && type == "callback" && self->sessions_.contains(physical_port) &&
              self->sessions_[physical_port].controller == controller) {
            self->sessions_[physical_port].starting = false;
          }
          reply(result, type);
        }, Qt::QueuedConnection);
      });
    }, Qt::QueuedConnection);
    return;
  }
  reply({{"success", false}, {"error", "DISCONNECTED"}}, "callback");
}
}  // namespace beamgo
