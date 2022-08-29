#ifndef MOTIONCONTROLLER_H
#define MOTIONCONTROLLER_H

#include <QObject>

enum class CtrlCmd {
    // Grbl ctrl cmd
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
};

class MotionController : public QObject
{
  Q_OBJECT
public:
  explicit MotionController(QObject *parent = nullptr);

signals:
  void sendCmdPacket(QString cmd_packet);

public slots:
  virtual void handleRawCmd(QString raw_cmd);
  virtual void handleCtrlCmd(CtrlCmd ctrl_cmd);
  virtual void handleJoggingCmd(qreal x, qreal y, bool relative);

private slots:

private:

};

#endif // MOTIONCONTROLLER_H
