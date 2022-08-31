#ifndef MOTIONCONTROLLER_H
#define MOTIONCONTROLLER_H

#include <QObject>
#include <connection/serial-port.h>

enum class MotionCtrlerCtrlCmd {
    // Grbl supported ctrl cmd
    kViewParserState,   // $G
    kUnlock,            // $X
    kHome,              // $H
    kViewGrblSettings,  // $$
    kViewBuildInfo,     // $I
    kViewStartupBlocks, // $N
    kToggleCheckMode,   // $C
    kStatusReport,      // ?
    kFeedHold,          // !
    kCycleStart,        // ~
    // xxx supported ctrl cmd
};

class MotionController : public QObject
{
  Q_OBJECT
public:
  explicit MotionController(QObject *parent = nullptr);

  void attachPort(SerialPort *port);
  void sendCmdPacket(QString cmd_packet);
  void sendCtrl(MotionCtrlerCtrlCmd ctrl_cmd);

signals:
  void disconnected();

public slots:
  //virtual void handleRawCmd(QString raw_cmd);
  //virtual void handleCtrlCmd(MotionCtrlerCtrlCmd ctrl_cmd);
  //virtual void handleJoggingCmd(qreal x, qreal y, bool relative);

private slots:

private:
  SerialPort* port_;
};

#endif // MOTIONCONTROLLER_H
