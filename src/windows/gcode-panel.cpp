#include "gcode-panel.h"
#include "ui_gcode-panel.h"
#include <QMessageBox>
#include <QRegularExpression>

#ifndef Q_OS_IOS

#include <QTimer>
#include <connection/serial-port.h>
#include <globals.h>

#include <QDebug>

#endif

GCodePanel::GCodePanel(QWidget *parent) :
     QFrame(parent),
     ui(new Ui::GCodePanel),
     BaseContainer() {
  ui->setupUi(this);
  initializeContainer();
}

void GCodePanel::loadSettings() {}

void GCodePanel::registerEvents() {
#ifndef Q_OS_IOS
  connect(ui->playBtn, &QAbstractButton::clicked, this, &GCodePanel::startBtnClicked);
  connect(ui->stopBtn, &QAbstractButton::clicked, this, &GCodePanel::stopBtnClicked);
  connect(ui->pauseBtn, &QAbstractButton::clicked, [=]() {
    // Pause / Resume
    if (job_state_ == Executor::State::kRunning) {
      emit pauseBtnClicked();
    } else {
      emit resumeBtnClicked();
    }
  });
  connect(ui->exportBtn, &QAbstractButton::clicked, this, &GCodePanel::exportGcode);
  connect(ui->importBtn, &QAbstractButton::clicked, this, &GCodePanel::importGcode);
  connect(ui->generateBtn, &QAbstractButton::clicked, this, &GCodePanel::generateGcode);
  connect(ui->gcodeText, &QPlainTextEdit::textChanged, this, &GCodePanel::checkGenerateGcode);

  connect(&serial_port, &SerialPort::connected, [=]() {
      qInfo() << "[SerialPort] Success connect!";
      ui->playBtn->setText(tr("Play"));
      if(!ui->gcodeText->toPlainText().isEmpty()) {
        ui->playBtn->setEnabled(true);
      }
  });
  connect(&serial_port, &SerialPort::disconnected, [=]() {
      qInfo() << "[SerialPort] Disconnected!";
      ui->playBtn->setText(tr("Play"));
      ui->playBtn->setEnabled(false);
  });
#endif
}

void GCodePanel::hideEvent(QHideEvent *event) {
  emit panelShow(false);
}

void GCodePanel::showEvent(QShowEvent *event) {
  emit panelShow(true);
}

void GCodePanel::checkGenerateGcode() {
  if(ui->gcodeText->toPlainText().isEmpty()) {
    ui->playBtn->setEnabled(false);
  }
  else if(serial_port.isOpen()) {
    ui->playBtn->setEnabled(true);
  }
}

void GCodePanel::showError(const QString &msg) {
  QMessageBox msgbox;
  msgbox.setText("Job Error");
  msgbox.setInformativeText(msg);
  msgbox.exec();
}

void GCodePanel::onJobStateChanged(Executor::State new_state) {
  job_state_ = new_state;
  switch (job_state_) {
    case Executor::State::kIdle:
      ui->pauseBtn->setEnabled(false);
      ui->playBtn->setEnabled(true);
      ui->stopBtn->setEnabled(false);
      break;
    case Executor::State::kRunning:
      qInfo() << "Running";
      ui->pauseBtn->setEnabled(true);
      ui->pauseBtn->setText(tr("Pause"));
      ui->playBtn->setEnabled(false);
      ui->stopBtn->setEnabled(true);
      break;
    case Executor::State::kPaused:
      qInfo() << "Paused";
      ui->pauseBtn->setEnabled(true);
      ui->pauseBtn->setText(tr("Resume"));
      break;
    case Executor::State::kCompleted:
      qInfo() << "Finished";
      ui->pauseBtn->setEnabled(false);
      ui->pauseBtn->setText(tr("Pause"));
      ui->playBtn->setEnabled(true);
      ui->stopBtn->setEnabled(false);
      onJobProgressChanged(100);
      break;
    case Executor::State::kStopped:
      qInfo() << "Stopped";
      ui->pauseBtn->setEnabled(false);
      ui->pauseBtn->setText(tr("Pause"));
      ui->playBtn->setEnabled(true);
      ui->stopBtn->setEnabled(false);
      onJobProgressChanged(0);
      break;
    default:
      break;
  }
  emit jobStatusReport(job_state_);
}

void GCodePanel::onJobProgressChanged(QVariant progress) {
  ui->progressBar->setValue(progress.toInt());
  ui->progressBarLabel->setText(ui->progressBar->text());
}

void GCodePanel::setGCode(const QString &gcode) {
  ui->gcodeText->setPlainText(gcode);
}

QString GCodePanel::getGCode() {
  return ui->gcodeText->toPlainText();
}

GCodePanel::~GCodePanel() {
  delete ui;
}

void GCodePanel::attachJob(QPointer<JobExecutor> job_executor) {
  // 1. Disconnect from current job first
  if (!job_executor_.isNull()) {
    disconnect(job_executor_, nullptr, this, nullptr);
  }
  // 2. Connect to new job
  qRegisterMetaType<Executor::State>(); // NOTE: This is necessary for passing custom type argument for signal/slot
  //connect(job_executor, &BaseJob::error, this, &GCodePanel::showError);
  connect(job_executor, &JobExecutor::progressChanged, this, &GCodePanel::onJobProgressChanged);
  connect(job_executor, &Executor::stateChanged, this, &GCodePanel::onJobStateChanged);
  job_executor_ = job_executor;
  onJobStateChanged(job_executor->getState());
  onJobProgressChanged(0);
}
