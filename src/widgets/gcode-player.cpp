#include "gcode-player.h"
#include "ui_gcode-player.h"
#include <QSerialPortInfo>
#include <QMessageBox>

GCodePlayer::GCodePlayer(QWidget *parent) :
     QFrame(parent),
     thread_(SerialPortThread(this)),
     ui(new Ui::GCodePlayer) {
  ui->setupUi(this);
  loadSettings();
  registerEvents();
}

void GCodePlayer::loadSettings() {
  const auto infos = QSerialPortInfo::availablePorts();
  for (const QSerialPortInfo &info : infos)
    ui->portComboBox->addItem(info.portName());
}

void GCodePlayer::registerEvents() {
  connect(ui->executeBtn, &QAbstractButton::clicked, [=]() {
    thread_.playGcode(ui->portComboBox->currentText(),
                      ui->baudComboBox->currentText().toInt(),
                      ui->gcodeText->toPlainText().split("\n"));
  });

  connect(&thread_, &SerialPortThread::error, this, &GCodePlayer::showError);
}

void GCodePlayer::showError(const QString &msg) {
  QMessageBox msgbox;
  msgbox.setText("Serial Port Error");
  msgbox.setInformativeText(msg);
  msgbox.exec();
}

void GCodePlayer::setGCode(const QString &gcode) {
  ui->gcodeText->setPlainText(gcode);
}

GCodePlayer::~GCodePlayer() {
  delete ui;
}
