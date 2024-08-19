#include "gcode-panel.h"
#include "ui_gcode-panel.h"
#include <QMessageBox>
#include <QRegularExpression>

#include <QTimer>
#include <QDebug>
#include <windows/mainwindow.h>
#include <executor/machine_job/gcode_job.h>

GCodePanel::GCodePanel(QWidget *parent, MainWindow* main_window) :
     main_window_(main_window),
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
      Q_EMIT pauseBtnClicked();
    } else {
      Q_EMIT resumeBtnClicked();
    }
  });
  connect(ui->exportBtn, &QAbstractButton::clicked, this, &GCodePanel::exportGcode);
  connect(ui->importBtn, &QAbstractButton::clicked, this, &GCodePanel::importGcode);
  connect(ui->generateBtn, &QAbstractButton::clicked, this, &GCodePanel::generateGcode);
  connect(ui->gcodeText, &QPlainTextEdit::textChanged, this, &GCodePanel::checkGenerateGcode);

  connect(main_window_, &MainWindow::activeMachineConnected, this, &GCodePanel::onEnableJobCtrl);
  connect(main_window_, &MainWindow::activeMachineDisconnected, this, &GCodePanel::onDisableJobCtrl);
#endif
}

void GCodePanel::hideEvent(QHideEvent *event) {
  Q_EMIT panelShow(false);
}

void GCodePanel::showEvent(QShowEvent *event) {
  Q_EMIT panelShow(true);
}

void GCodePanel::onEnableJobCtrl() {
  enabled_ = true;
  ui->playBtn->setText(tr("Play"));
  if(!ui->gcodeText->toPlainText().isEmpty()) {
    ui->playBtn->setEnabled(true);
  }
}

void GCodePanel::onDisableJobCtrl() {
  enabled_ = false;
  ui->playBtn->setText(tr("Play"));
  ui->playBtn->setEnabled(false);
}

void GCodePanel::checkGenerateGcode() {
  if(ui->gcodeText->toPlainText().isEmpty()) {
    ui->playBtn->setEnabled(false);
  } else {
    ui->playBtn->setEnabled(true && enabled_);
  }
}

void GCodePanel::onJobStateChanged(Executor::State new_state) {
  job_state_ = new_state;
  switch (job_state_) {
    case Executor::State::kIdle:
      ui->pauseBtn->setEnabled(false && enabled_);
      ui->playBtn->setEnabled(true && enabled_);
      ui->stopBtn->setEnabled(false && enabled_);
      break;
    case Executor::State::kRunning:
      qInfo() << "Running";
      ui->pauseBtn->setEnabled(true && enabled_);
      ui->pauseBtn->setText(tr("Pause"));
      ui->playBtn->setEnabled(false && enabled_);
      ui->stopBtn->setEnabled(true && enabled_);
      break;
    case Executor::State::kPaused:
      qInfo() << "Paused";
      ui->pauseBtn->setEnabled(true && enabled_);
      ui->pauseBtn->setText(tr("Resume"));
      break;
    case Executor::State::kCompleted:
      qInfo() << "Finished";
      ui->pauseBtn->setEnabled(false && enabled_);
      ui->pauseBtn->setText(tr("Pause"));
      ui->playBtn->setEnabled(true && enabled_);
      ui->stopBtn->setEnabled(false && enabled_);
      onJobProgressChanged(100);
      break;
    case Executor::State::kStopped:
      qInfo() << "Stopped";
      ui->pauseBtn->setEnabled(false && enabled_);
      ui->pauseBtn->setText(tr("Pause"));
      ui->playBtn->setEnabled(true && enabled_);
      ui->stopBtn->setEnabled(false && enabled_);
      onJobProgressChanged(0);
      break;
    default:
      break;
  }
  Q_EMIT jobStatusReport(job_state_);
}

void GCodePanel::onJobProgressChanged(QVariant progress) {
  ui->progressBar->setValue(progress.toInt());
  ui->progressBarLabel->setText(ui->progressBar->text());
}

void GCodePanel::setGCode(const QString &gcode) {
  ui->gcodeText->setPlainText(gcode);
  gcode_list_ = gcode.split('\n');
  auto progress_dialog = new QProgressDialog(
    tr("Estimating task time..."),  
    tr("Cancel"), 
    0, gcode_list_.size() - 1, 
    this);
  this->timestamp_list_ = MachineJob::calcRequiredTime(gcode_list_, progress_dialog);
  delete progress_dialog;
}

const QStringList& GCodePanel::getGCodeList() {
  return gcode_list_;
}

const QList<Timestamp>& GCodePanel::getTimestampList() {
  return timestamp_list_;
}

Timestamp GCodePanel::getEstimatedTime() {
  if (!timestamp_list_.empty()) {
    return timestamp_list_.last();
  }
  return Timestamp();
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
  connect(job_executor, &JobExecutor::progressChanged, this, &GCodePanel::onJobProgressChanged);
  connect(job_executor, &Executor::stateChanged, this, &GCodePanel::onJobStateChanged);
  job_executor_ = job_executor;
  onJobStateChanged(job_executor->getState());
  onJobProgressChanged(0);
}
