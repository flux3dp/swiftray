#include <QPainter>
#include <QScrollBar>
#include <QSlider>
#include <QDebug>
#include "preview-window.h"
#include "ui_preview-window.h"
#include <windows/osxwindow.h>

#include <QGraphicsView>
#include <QGestureEvent>
#include <QGraphicsLineItem>

class PathGraphicsPreview: public QGraphicsView {
public:
    PathGraphicsPreview(QGraphicsScene *scene = nullptr, QWidget *parent = nullptr);
    bool event(QEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
private:

    void gestureHandler(QGestureEvent *ge);
    void pinchGestureHandler(QPinchGesture *pg);
    void keyHandler(QKeyEvent *ke);

    //qreal rotationAngle = 0;
    qreal scaleFactor = 1;

    bool zooming_ = false;
    QPointF zoom_fixed_point_scene_; // the cursor location in scene when start zooming
    QPointF zoom_fixed_point_view_;  // initial zoom cursor location in view coord
    bool is_holding_ctrl_  = false;
};

PathGraphicsPreview::PathGraphicsPreview(QGraphicsScene *scene, QWidget *parent)
        : QGraphicsView(scene, parent)
{
  // Background color outside working area
  setBackgroundBrush(QBrush(
    isDarkMode() ? QColor("#454545") : QColor("#F0F0F0"), 
    Qt::SolidPattern)
  );
  
  setDragMode(ScrollHandDrag); // mouse drag

  grabGesture(Qt::PinchGesture);

  //setAttribute(Qt::WA_AcceptTouchEvents);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn); // Qt::ScrollBarAlwaysOff
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);   // Qt::ScrollBarAlwaysOff
}

bool PathGraphicsPreview::event(QEvent *e) {

  if (e->type() == QEvent::Gesture) {
    gestureHandler(static_cast<QGestureEvent*>(e));
  }
  else if (e->type() == QEvent::KeyPress) {
    keyHandler(static_cast<QKeyEvent*>(e));
  }
  else if (e->type() == QEvent::KeyRelease) {
    keyHandler(static_cast<QKeyEvent*>(e));
  }
  return QGraphicsView::event(e);
}

void PathGraphicsPreview::wheelEvent(QWheelEvent *e) {
  if (is_holding_ctrl_) {
    double new_scale = 1 + (double)e->angleDelta().y() / 8 / this->height();
    if (this->scaleFactor * new_scale >= 1) {
      this->scaleFactor *= new_scale;
      this->scale(new_scale, new_scale);
    }
    else {
      this->scale(1.0/this->scaleFactor, 1.0/this->scaleFactor);
      this->scaleFactor = 1;
    }
    return;
  }
  return QGraphicsView::wheelEvent(e);
}

void PathGraphicsPreview::gestureHandler(QGestureEvent *gesture_event) {
  // NOTE: For MacOSX, only pinch gesture works (pan/swipe gesture don't work)
  if (QGesture *pinch = gesture_event->gesture(Qt::PinchGesture)) {
    pinchGestureHandler(static_cast<QPinchGesture *>(pinch));
  }
}

void PathGraphicsPreview::pinchGestureHandler(QPinchGesture *pinch_gesture) {
  QPinchGesture::ChangeFlags changeFlags = pinch_gesture->changeFlags();
  /*
  if (changeFlags & QPinchGesture::RotationAngleChanged) {
    qreal rotationDelta = pinch_gesture->rotationAngle() - pinch_gesture->lastRotationAngle();
    rotationAngle += rotationDelta;
    //qInfo() << "pinchTriggered(): rotate by" << rotationDelta << "->" << rotationAngle;
  }
  */
  if (changeFlags & QPinchGesture::ScaleFactorChanged) {

    if ( ! zooming_) {
      // Start of pinch zooming (store necessary states)
      zoom_fixed_point_view_ = pinch_gesture->centerPoint();
      zoom_fixed_point_scene_ = mapToScene(pinch_gesture->centerPoint().toPoint());
      zooming_ = true;
    }
    if (this->scaleFactor * pinch_gesture->scaleFactor() >= 1) {

      this->scaleFactor *= pinch_gesture->scaleFactor();
      this->scale(pinch_gesture->scaleFactor(), pinch_gesture->scaleFactor());

      auto scaled_target_point_in_view = mapFromScene(zoom_fixed_point_scene_);
      auto delta = scaled_target_point_in_view - zoom_fixed_point_view_;

      this->horizontalScrollBar()->setValue(this->horizontalScrollBar()->value() + delta.x());
      this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() + delta.y());
    }

  }
  if (pinch_gesture->state() == Qt::GestureFinished) {
    zooming_ = false;
  }
}

void PathGraphicsPreview::keyHandler(QKeyEvent *ke) {
  if (ke->modifiers() & Qt::ControlModifier) {
    is_holding_ctrl_ = ke->modifiers() & Qt::ControlModifier;
  }
  else if(is_holding_ctrl_) {
    is_holding_ctrl_ = ke->modifiers() & Qt::ControlModifier;
  }
}

/**
 * @brief
 * @param parent
 * @param width The width of canvas (= machine working area) in the physical (real) unit (e.g. mm))
 *              instead of virtual document size (e.g. 3000)
 * @param height The height of canvas (= machine working area) in the physical (real) unit (e.g. mm))
 *               instead of virtual document size (e.g. 2000)
 */
PreviewWindow::PreviewWindow(QWidget *parent, int width, int height) :
     QDialog(parent),
     ui(new Ui::PreviewWindow),
     preview_path_(nullptr),
     BaseContainer() {
  ui->setupUi(this);
  initializeContainer();

  auto scene = new QGraphicsScene(this);
  scene->setSceneRect(0, 0, width, height); // in unit of mm

  path_graphics_view_ = new PathGraphicsPreview(scene, ui->frame);

  Q_ASSERT_X(ui->frame->layout() != nullptr, "PreviewWindow", "Frame container must exist");
  static_cast<QBoxLayout*>(ui->frame->layout())->insertWidget(0, path_graphics_view_);

  registerEvents();
}

PreviewWindow::~PreviewWindow() {
  delete ui;
}

void PreviewWindow::registerEvents() {
  connect(ui->progress, &QSlider::valueChanged, [=](int value) {
      // Clear all paths on the GraphicsScene
      this->path_graphics_view_->scene()->clear();
      // Add background
      this->path_graphics_view_->scene()->addRect(
              this->path_graphics_view_->scene()->sceneRect(),
              QPen{Qt::white, 0, Qt::SolidLine},
              QBrush{Qt::white});

      // Add paths on the GraphicsScene
      QPointF current_pos = QPointF(3, 3);
      auto paths = preview_path_->paths();
      auto pen_black = QPen(QColor{0, 0, 0, 255}, 0, Qt::SolidLine); // zero width -> cosmetic pen
      auto pen_red = QPen(QColor{255, 0, 0, 155}, 0, Qt::SolidLine); // zero width -> cosmetic pen
      QPainterPath red_path;
      QPainterPath black_path;
      for (int i = 0; i < ui->progress->value(); i++) {
        if (paths[i].power_ > 0) {
          red_path.moveTo(paths[i].target_);
          black_path.lineTo(paths[i].target_);
        } else {
          red_path.lineTo(paths[i].target_);
          black_path.moveTo(paths[i].target_);
        }
        current_pos = paths[i].target_;
      }
      this->path_graphics_view_->scene()->addPath(black_path, pen_black);
      this->path_graphics_view_->scene()->addPath(red_path, pen_red);

      auto end_pen = QPen(Qt::red, 2, Qt::SolidLine);
      end_pen.setCosmetic(true);
      this->path_graphics_view_->scene()->addEllipse(current_pos.x() - 2, current_pos.y() - 2, 4, 4, end_pen);

      update();
  });
}

void PreviewWindow::showEvent(QShowEvent *event) {
  QDialog::showEvent(event);
  path_graphics_view_->fitInView(path_graphics_view_->sceneRect(), Qt::KeepAspectRatio);
}

void PreviewWindow::resizeEvent(QResizeEvent *event) {
  QDialog::resizeEvent(event);
  path_graphics_view_->fitInView(path_graphics_view_->sceneRect(), Qt::KeepAspectRatio);
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

void PreviewWindow::setRequiredTime(const QTime &required_time) {
  ui->requiredTimeLabel->setText(required_time.toString("hh:mm:ss"));
}
