#include <QPainter>
#include <QScrollBar>
#include <QSlider>
#include <QDebug>
#include "preview-window.h"
#include "ui_preview-window.h"

#include <QGraphicsView>
#include <QGestureEvent>
#include <QGraphicsLineItem>

class PathGraphicsPreview: public QGraphicsView {
public:
    PathGraphicsPreview(QGraphicsScene *scene = nullptr, QWidget *parent = nullptr);
    bool event(QEvent *e) override;
private:

    void gestureHandler(QGestureEvent *ge);
    void pinchGestureHandler(QPinchGesture *pg);

    //qreal rotationAngle = 0;
    qreal scaleFactor = 1;

    bool zooming_ = false;
    QPointF zoom_fixed_point_scene_; // the cursor location in scene when start zooming
    QPointF zoom_fixed_point_view_;  // initial zoom cursor location in view coord
};

PathGraphicsPreview::PathGraphicsPreview(QGraphicsScene *scene, QWidget *parent)
        : QGraphicsView(scene, parent)
{
  setDragMode(ScrollHandDrag); // mouse drag

  grabGesture(Qt::PinchGesture);
  //setBackgroundBrush(Qt::green);
  //setStyleSheet("border: 1px solid red");

  //setAttribute(Qt::WA_AcceptTouchEvents);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn); // Qt::ScrollBarAlwaysOff
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);   // Qt::ScrollBarAlwaysOff

  //setTransformationAnchor(QGraphicsView::AnchorViewCenter);
  //setResizeAnchor(QGraphicsView::AnchorViewCenter);
}

bool PathGraphicsPreview::event(QEvent *e) {

  if (e->type() == QEvent::Gesture) {
    gestureHandler(static_cast<QGestureEvent*>(e));
  }
  return QGraphicsView::event(e);
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
    //qInfo() << "pinchTriggered(): zoom by" <<
    //    pinch_gesture->scaleFactor() << "->" << currentStepScaleFactor;
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
    //qInfo() << "gesture finished";
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
     progress_(50),
     preview_path_(nullptr),
     BaseContainer() {
  ui->setupUi(this);
  setWindowTitle(tr("Preview Path"));
  initializeContainer();

  auto scene = new QGraphicsScene(this);
  //scene->setBackgroundBrush(Qt::gray);
  scene->setSceneRect(0, 0, width, height); // in unit of mm

  path_graphics_view_ = new PathGraphicsPreview(scene, ui->frame);

  Q_ASSERT_X(ui->frame->layout() != nullptr, "PreviewWindow", "Frame container must exist");
  static_cast<QHBoxLayout*>(ui->frame->layout())->insertWidget(0, path_graphics_view_);

  registerEvents();

}

PreviewWindow::~PreviewWindow() {
  delete ui;
}

void PreviewWindow::registerEvents() {
  connect(ui->progress, &QSlider::valueChanged, [=](int value) {
      progress_ = value;

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
      auto pen_black = QPen(Qt::black, 0, Qt::SolidLine);
      auto pen_red = QPen(Qt::red, 0, Qt::SolidLine);
      for (int i = 0; i < ui->progress->value(); i++) {
        auto line = new QGraphicsLineItem(QLineF(current_pos, paths[i].target_));
        //line->setFlag(QGraphicsItem::ItemIsMovable);
        //line->setFlag(QGraphicsItem::ItemIsSelectable);
        if (paths[i].power_ > 0) {
          line->setPen(pen_black);
        } else {
          line->setPen(pen_red);
        }
        this->path_graphics_view_->scene()->addItem(line);
        current_pos = paths[i].target_;
      }
      auto end_pen = QPen(Qt::red, 2, Qt::SolidLine);
      end_pen.setCosmetic(true);
      this->path_graphics_view_->scene()->addEllipse(current_pos.x() - 2, current_pos.y() - 2, 4, 4, end_pen);

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

void PreviewWindow::setRequiredTime(const QString &required_time) {
  ui->requiredTime->setText(tr("Required Time")+": "+required_time);
}
