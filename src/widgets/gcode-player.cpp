#include "gcode-player.h"
#include "ui_gcode-player.h"
#include <QSerialPortInfo>

GCodePlayer::GCodePlayer(QWidget *parent) :
     QFrame(parent),
     ui(new Ui::GCodePlayer) {
  ui->setupUi(this);
  loadSettings();
}

void GCodePlayer::loadSettings() {
  const auto infos = QSerialPortInfo::availablePorts();
  for (const QSerialPortInfo &info : infos)
    ui->portComboBox->addItem(info.portName());
}

void GCodePlayer::setGCode(const QString &gcode) {
  ui->gcodeText->setPlainText(gcode);
}

GCodePlayer::~GCodePlayer() {
  delete ui;
}
