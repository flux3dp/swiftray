#include "job-dashboard-dialog.h"
#include "ui_job-dashboard-dialog.h"

#include <QDebug>
#include <windows/osxwindow.h>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsView>


JobDashboardDialog::JobDashboardDialog(const QPixmap &preview, QWidget *parent) :
        QDialog(parent),
        preview_(preview),
        ui(new Ui::JobDashboardDialog)
{
  ui->setupUi(this);

  setAttribute(Qt::WA_DeleteOnClose);
  ui->statusLabel->setText(Executor::stateToString(job_state_));
  ui->estTotalRequiredTime->setText(Timestamp{0, 0}.toString());
  QGraphicsScene *scene = new QGraphicsScene(ui->canvasView);
  scene->addPixmap(preview_);
  ui->canvasView->setScene(scene);
  ui->stopBtn->setEnabled(false);
  ui->startBtn->setEnabled(true);

  initializeContainer();
}

JobDashboardDialog::JobDashboardDialog(
  Timestamp total_required_time, const QPixmap &preview, QWidget *parent) :
    QDialog(parent),
    preview_(preview),
    ui(new Ui::JobDashboardDialog)
{
  ui->setupUi(this);

  setAttribute(Qt::WA_DeleteOnClose);

  ui->statusLabel->setText(Executor::stateToString(job_state_));
  ui->estTotalRequiredTime->setText(total_required_time.toString());
  QGraphicsScene *scene = new QGraphicsScene(ui->canvasView);
  scene->addPixmap(preview_);
  ui->canvasView->setScene(scene);
  ui->stopBtn->setEnabled(false);
  ui->startBtn->setEnabled(true);

  initializeContainer();
}

JobDashboardDialog::~JobDashboardDialog()
{
    delete ui;
}

void JobDashboardDialog::registerEvents() {
  connect(ui->startBtn, &QToolButton::clicked, this, [=](){
    if (job_state_ == Executor::State::kIdle ||
        job_state_ == Executor::State::kCompleted ||
        job_state_ == Executor::State::kStopped) {
      emit startBtnClicked(); // try to start a new job
    } else if (job_state_ == Executor::State::kRunning) {
      emit pauseBtnClicked();
    } else if (job_state_ == Executor::State::kPaused) {
      emit resumeBtnClicked();
    }
  });
  connect(ui->stopBtn, &QToolButton::clicked, this, &JobDashboardDialog::stopBtnClicked);
}

void JobDashboardDialog::loadStyles() {
  ui->startBtn->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-start.png" : ":/resources/images/icon-start.png"));
  ui->startBtn->setGeometry(ui->startBtn->geometry().left() + 100, this->size().height() - 100, 50, 50);
  ui->startBtn->setStyleSheet("QToolButton { border: none; } QToolButton::hover { border: none; }");
  ui->startBtn->setIconSize(QSize{50, 50});

  ui->stopBtn->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-stop.png" : ":/resources/images/icon-stop.png"));
  ui->stopBtn->setGeometry(ui->stopBtn->geometry().left() + 100, this->size().height() - 100, 50, 50);
  ui->stopBtn->setStyleSheet("QToolButton { border: none; } QToolButton::hover { border: none; }");
  ui->stopBtn->setIconSize(QSize{50, 50});
}

QPixmap JobDashboardDialog::getPreview() {
  return preview_;
}

void JobDashboardDialog::onJobStateChanged(Executor::State state) {
  job_state_ = state;

  ui->statusLabel->setText(Executor::stateToString(job_state_));

  // TODO: Change button states
  switch (job_state_) {
    case Executor::State::kIdle:
      ui->startBtn->setEnabled(true); // act as start btn
      ui->startBtn->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-start.png" : ":/resources/images/icon-start.png"));
      ui->stopBtn->setEnabled(false);
      break;
    case Executor::State::kRunning:
      ui->startBtn->setEnabled(true); // act as pause btn
      ui->startBtn->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-pause.png" : ":/resources/images/icon-pause.png"));
      ui->stopBtn->setEnabled(true);
      break;
    case Executor::State::kPaused:
      ui->startBtn->setEnabled(true); // act as resume btn
      ui->startBtn->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-start.png" : ":/resources/images/icon-start.png"));
      ui->stopBtn->setEnabled(true);
      break;
    case Executor::State::kCompleted:
      ui->startBtn->setEnabled(true); // act as start (restart) btn
      ui->startBtn->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-start.png" : ":/resources/images/icon-start.png"));
      ui->stopBtn->setEnabled(false);
      onJobProgressChanged(100);
      break;
    case Executor::State::kStopped:
      ui->startBtn->setEnabled(true); // act as start (restart) btn
      ui->startBtn->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-start.png" : ":/resources/images/icon-start.png"));
      ui->stopBtn->setEnabled(false);
      onJobProgressChanged(0);
      break;
    default:
      break;
  }
  emit jobStatusReport(job_state_);
}

void JobDashboardDialog::onJobProgressChanged(QVariant progress) {
  ui->progressBar->setValue(progress.toInt());
  ui->progressLabel->setText(ui->progressBar->text());
}

void JobDashboardDialog::onElapsedTimeChanged(Timestamp new_timestamp) {
  Timestamp time_left{0, 0};
  time_left = time_left.addSecs(new_timestamp.secsTo(total_required_time_));
  ui->timeLeft->setText(time_left.toString() + " left");
}

void JobDashboardDialog::attachJob(QPointer<JobExecutor> job_executor) {
  // 1. Disconnect from current job first
  if (!job_executor_.isNull()) {
    disconnect(job_executor_, nullptr, this, nullptr);
  }
  // 2. Connect to new job
  qRegisterMetaType<Executor::State>();  // NOTE: This is necessary for passing custom type argument for signal/slot
  connect(job_executor, &Executor::stateChanged, this, &JobDashboardDialog::onJobStateChanged);
  connect(job_executor, &JobExecutor::progressChanged, this, &JobDashboardDialog::onJobProgressChanged);
  connect(job_executor, &JobExecutor::elapsedTimeChanged, this, &JobDashboardDialog::onElapsedTimeChanged);
  job_executor_ = job_executor;
  // Initialize displayed job info
  total_required_time_ = job_executor->getTotalRequiredTime();
  ui->estTotalRequiredTime->setText(total_required_time_.toString());
  onJobStateChanged(job_executor->getState());
  onJobProgressChanged(job_executor->getProgress());
  onElapsedTimeChanged(job_executor->getElapsedTime());
}

void JobDashboardDialog::showEvent(QShowEvent *event) {
  QDialog::showEvent(event);
  ui->canvasView->fitInView(ui->canvasView->sceneRect(), Qt::KeepAspectRatio);
}

void JobDashboardDialog::resizeEvent(QResizeEvent *event) {
  ui->canvasView->fitInView(ui->canvasView->sceneRect(), Qt::KeepAspectRatio);
}
