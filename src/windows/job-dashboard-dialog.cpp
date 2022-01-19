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
  ui->statusLabel->setText(BaseJob::statusToString(status_));
  ui->estTotalRequiredTime->setText(QTime{0, 0}.toString("hh:mm:ss"));
  QGraphicsScene *scene = new QGraphicsScene(ui->canvasView);
  scene->addPixmap(preview_);
  ui->canvasView->setScene(scene);

  initializeContainer();
}

JobDashboardDialog::JobDashboardDialog(QTime total_required_time, const QPixmap &preview, QWidget *parent) :
    QDialog(parent),
    preview_(preview),
    ui(new Ui::JobDashboardDialog)
{
  ui->setupUi(this);

  setAttribute(Qt::WA_DeleteOnClose);

  ui->statusLabel->setText(BaseJob::statusToString(status_));
  ui->estTotalRequiredTime->setText(total_required_time.toString("hh:mm:ss"));
  QGraphicsScene *scene = new QGraphicsScene(ui->canvasView);
  scene->addPixmap(preview_);
  ui->canvasView->setScene(scene);

  initializeContainer();
}

JobDashboardDialog::~JobDashboardDialog()
{
    delete ui;
}

void JobDashboardDialog::registerEvents() {
  connect(ui->startBtn, &QToolButton::clicked, this, [=](){
    if (status_ == BaseJob::Status::READY ||
        status_ == BaseJob::Status::FINISHED ||
        status_ == BaseJob::Status::ERROR_STOPPED) {
      emit startBtnClicked(); // try to start a new job
    } else if (status_ == BaseJob::Status::RUNNING) {
      emit pauseBtnClicked();
    } else if (status_ == BaseJob::Status::PAUSED) {
      emit resumeBtnClicked();
    }
  });
  connect(ui->stopBtn, &QToolButton::clicked, this, &JobDashboardDialog::stopBtnClicked);
}

void JobDashboardDialog::loadStyles() {
  ui->startBtn->setIcon(QIcon(isDarkMode() ? ":/images/dark/icon-start.png" : ":/images/icon-start.png"));
  ui->startBtn->setGeometry(ui->startBtn->geometry().left() + 100, this->size().height() - 100, 50, 50);
  ui->startBtn->setStyleSheet("QToolButton { border: none; } QToolButton::hover { border: none; }");

  //ui->stopBtn->setIcon(QIcon(isDarkMode() ? ":/images/dark/icon-stop.png" : ":/images/icon-stop.png"));
  //ui->stopBtn->setGeometry(ui->stopBtn->geometry().left() + 140, this->size().height() - 140, 70, 70);
  //ui->stopBtn->setStyleSheet("QToolButton { border: none; } QToolButton::hover { border: none; }");
}

void JobDashboardDialog::onStatusChanged(BaseJob::Status status) {
  status_ = status;
  ui->statusLabel->setText(BaseJob::statusToString(status_));

  // TODO: Change button states
  switch (status_) {
    case BaseJob::Status::READY:
      ui->startBtn->setEnabled(true); // act as start btn
      ui->stopBtn->setEnabled(false);
      break;
    case BaseJob::Status::RUNNING:
      ui->startBtn->setEnabled(true); // act as pause btn
      ui->stopBtn->setEnabled(true);
      break;
    case BaseJob::Status::PAUSED:
      ui->startBtn->setEnabled(true); // act as resume btn
      ui->stopBtn->setEnabled(true);
      break;
    case BaseJob::Status::FINISHED:
      ui->startBtn->setEnabled(true); // act as start (restart) btn
      ui->stopBtn->setEnabled(false);
      onProgressChanged(100);
      break;
    case BaseJob::Status::STOPPING:
    case BaseJob::Status::ERROR_STOPPING:
      ui->startBtn->setEnabled(false);
      ui->stopBtn->setEnabled(false);
      break;
    case BaseJob::Status::STOPPED:
    case BaseJob::Status::ERROR_STOPPED:
      ui->startBtn->setEnabled(true); // act as start (restart) btn
      ui->stopBtn->setEnabled(false);
      onProgressChanged(0);
      break;
    default:
      break;
  }
}

void JobDashboardDialog::onProgressChanged(QVariant progress) {
  ui->progressBar->setValue(progress.toInt());
  ui->progressLabel->setText(ui->progressBar->text());
}

void JobDashboardDialog::onElapsedTimeChanged(QTime new_timestamp) {
  QTime time_left{0, 0};
  time_left = time_left.addSecs(new_timestamp.secsTo(job_->getTotalRequiredTime()));
  ui->timeLeft->setText(time_left.toString("hh:mm:ss") + " left");
}

void JobDashboardDialog::attachJob(BaseJob *job) {
  job_ = job;
  //qRegisterMetaType<BaseJob::Status>();  // NOTE: This is necessary for passing enum class argument for signal/slot
  connect(job_, &BaseJob::statusChanged, this, &JobDashboardDialog::onStatusChanged);
  connect(job_, &BaseJob::progressChanged, this, &JobDashboardDialog::onProgressChanged);
  connect(job_, &BaseJob::elapsedTimeChanged, this, &JobDashboardDialog::onElapsedTimeChanged);

  // Initialize displayed job info
  ui->estTotalRequiredTime->setText(job_->getTotalRequiredTime().toString("hh:mm:ss"));
  onStatusChanged(job_->status());
  onProgressChanged(0);
  onElapsedTimeChanged(QTime{0,0});
}

void JobDashboardDialog::showEvent(QShowEvent *event) {
  QDialog::showEvent(event);
  ui->canvasView->fitInView(ui->canvasView->sceneRect(), Qt::KeepAspectRatio);
}

void JobDashboardDialog::resizeEvent(QResizeEvent *event) {
  ui->canvasView->fitInView(ui->canvasView->sceneRect(), Qt::KeepAspectRatio);
}
