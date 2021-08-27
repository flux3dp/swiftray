#include <widgets/components/base-graphicsview.h>
#include <QScrollBar>
#include <QPainterPath>

#include <QDebug>

BaseGraphicsView::BaseGraphicsView(QWidget *parent)
        : QGraphicsView(parent)
{
  setDragMode(ScrollHandDrag); // mouse drag
  grabGesture(Qt::PinchGesture);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn); // Qt::ScrollBarAlwaysOff
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);   // Qt::ScrollBarAlwaysOff
}

BaseGraphicsView::BaseGraphicsView(QGraphicsScene *scene, QWidget *parent)
        : QGraphicsView(scene, parent)
{
  setDragMode(ScrollHandDrag); // mouse drag
  grabGesture(Qt::PinchGesture);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn); // Qt::ScrollBarAlwaysOff
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);   // Qt::ScrollBarAlwaysOff
}

bool BaseGraphicsView::event(QEvent *e) {
  if (e->type() == QEvent::Gesture) {
    gestureHandler(static_cast<QGestureEvent*>(e));
  }
  return QGraphicsView::event(e);
}

void BaseGraphicsView::gestureHandler(QGestureEvent *gesture_event) {
  // NOTE: For MacOSX, only pinch gesture works (pan/swipe gesture don't work)
  if (QGesture *pinch = gesture_event->gesture(Qt::PinchGesture)) {
    pinchGestureHandler(static_cast<QPinchGesture *>(pinch));
  }
}

void BaseGraphicsView::pinchGestureHandler(QPinchGesture *pinch_gesture) {
  QPinchGesture::ChangeFlags changeFlags = pinch_gesture->changeFlags();

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
