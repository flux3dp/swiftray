#include <QPainter>
#include <QScrollBar>
#include <QSlider>
#include <QDebug>
#include "preview-window.h"
#include "ui_preview-window.h"

PreviewWindow::PreviewWindow(QWidget *parent) :
     QDialog(parent),
     ui(new Ui::PreviewWindow),
     progress_(50),
     preview_path_(nullptr),
     BaseContainer() {
  ui->setupUi(this);
  setWindowTitle("Preview Path");
  initializeContainer();
}

PreviewWindow::~PreviewWindow() {
  delete ui;
}

void PreviewWindow::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  // TODO ( Refactor to better paradigm )
  QRectF screen = QRect(ui->pathFrame->parentWidget()->mapToParent(QPoint(4, 1)),
                        ui->pathFrame->parentWidget()->mapToParent(
                             QPoint(ui->pathFrame->width(), ui->pathFrame->height())));
  float scale = (float) screen.width() / 300.0F;
  painter.setTransform(QTransform().translate(screen.x(), screen.y()).scale(scale, scale));
  QPointF current_pos = QPointF(0, 0);
  auto paths = preview_path_->paths();
  auto pen_black = QPen(Qt::black, 0, Qt::SolidLine);
  auto pen_red = QPen(Qt::red, 0, Qt::SolidLine);
  for (int i = 0; i < ui->progress->value(); i++) {
    if (paths[i].power_ > 0) {
      painter.setPen(pen_black);
    } else {
      painter.setPen(pen_red);
    }
    painter.drawLine(current_pos, paths[i].target_);
    current_pos = paths[i].target_;
  }
  auto end_pen = QPen(Qt::red, 2, Qt::SolidLine);
  end_pen.setCosmetic(true);
  painter.setPen(end_pen);
  painter.drawEllipse(current_pos, 2, 2);
  QWidget::paintEvent(event);
}

void PreviewWindow::registerEvents() {
  connect(ui->progress, &QSlider::valueChanged, [=](int value) {
    progress_ = value;
    update();
  });
}

PreviewGenerator *PreviewWindow::previewPath() const {
  return preview_path_.get();
}

void PreviewWindow::setPreviewPath(std::shared_ptr<PreviewGenerator> &preview_path) {
  preview_path_ = preview_path;
  ui->progress->setMaximum(preview_path_->paths().size());
  ui->progress->setValue(preview_path_->paths().size());
  update();
}
