#pragma once

#include "controller.h"
#include <QMap>
#include <QSet>
#include <QThread>

namespace beamgo {
class Manager : public QObject {
  Q_OBJECT
 public:
  explicit Manager(QObject* parent = nullptr);
  ~Manager();
  QJsonArray devices() const;
  bool hasActiveJob() const;
  void action(const QString& port, const QString& action, const QJsonObject& params, Reply reply);

 private:
  struct Session {
    Controller* controller;
    QThread* thread;
    QJsonObject info;
    bool starting = false;
  };
  QMap<QString, Session> sessions_;
  QSet<QThread*> threads_;
  QTimer scan_{this};
  void scan();
  void retire(const QString& port);
};
}  // namespace beamgo
