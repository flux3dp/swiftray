#pragma once

#include "worker.h"
#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <machine/machine.h>
#include <canvas/canvas.h>
#include <toolpath_exporter/generators/gcode-generator.h>
#include <toolpath_exporter/stl-utils.h>

class SwiftrayServer : public QObject {
  Q_OBJECT

public:
  explicit SwiftrayServer(quint16 port, QObject* parent = nullptr);

private Q_SLOTS:
  void onNewConnection();
  void processMessage(const QString& message);
  void processBinaryMessage(const QByteArray& message);

Q_SIGNALS:
  void interruptWorker(QPointer<QWebSocket> socket_ptr);
  void sendTaskToWorker(QWebSocket* socket, const QString& id, const QString& action, const QJsonValue& params);

private:
  QMap<QString, Machine*> machine_map_;
  QWebSocketServer* m_server;
  Machine* m_machine;
  QString m_buffer;
  Canvas* m_canvas = nullptr;
  /**
   * Meshes of the STL objects of the current document, keyed by the id of their placeholder rect.
   * Loaded together with the SVG and kept until the next loadSVG; a model can be tens of MB.
   */
  QMap<QString, stl::Mesh> m_stl_objects;
  /** BSPC objects of the current document, in each frontend object's local millimetre space. */
  QMap<QString, stl::PointCloud> m_point_cloud_objects;
  QString m_thumbnail;
  QStringList gcode_list_;
  bool m_rotary_mode;
  double m_time_cost = 0;
  std::mutex canvas_mutex_;
  QThread* workerThread = nullptr;
  Worker* worker;
  friend class Worker;

  void handleDevicesAction(QWebSocket* socket, const QString& id, const QString& action, const QJsonValue& params);
  void handleDeviceSpecificAction(QWebSocket* socket, const QString& id, const QString& action, const QJsonValue& params, const QString& port);
  bool handleParserAction(QWebSocket* socket, const QString& id, const QString& action, const QJsonValue& params);
  void handleSystemAction(QWebSocket* socket, const QString& id, const QString& action, const QJsonValue& params);
  void sendData(QWebSocket* socket, const QString& id, const QJsonObject& result, const QString& type);
  void sendCallback(QWebSocket* socket, const QString& id, const QJsonObject& result);
  void sendEvent(QWebSocket* socket, const QString& event, const QJsonObject& data);
  bool startFraming(const QJsonValue& params);
  void setupWorker();
  Machine* getMachine();
  QJsonArray getDeviceList();
};
