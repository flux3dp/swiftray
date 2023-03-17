#include <QPainterPath>
#include <canvas/controls/oval.h>
#include <cmath>
#include <shape/path-shape.h>
#include <canvas/canvas.h>

using namespace Controls;

bool Oval::isActive() {
  return canvas().mode() == Canvas::Mode::OvalDrawing;
}

bool Oval::mouseMoveEvent(QMouseEvent *e) {
  pressed_pos_ = document().mousePressedCanvasCoord();
  mouse_pos_ = document().getCanvasCoord(e->pos());
  if (scale_locked_) {
    QPointF adjusted_pos;
    if ((mouse_pos_.y() - pressed_pos_.y()) < (mouse_pos_.x() - pressed_pos_.x())) {
      adjusted_pos.setX(mouse_pos_.x());
      adjusted_pos.setY(pressed_pos_.y() + mouse_pos_.x() - pressed_pos_.x());
    } else {
      adjusted_pos.setX(pressed_pos_.x() + mouse_pos_.y() - pressed_pos_.y());
      adjusted_pos.setY(mouse_pos_.y());
    }

    rect_ = QRectF(pressed_pos_, adjusted_pos);
  } else {
    rect_ = QRectF(pressed_pos_, mouse_pos_);
  }
  return true;
}

bool Oval::mouseReleaseEvent(QMouseEvent *e) {
  QPainterPath path;
  path.moveTo((rect_.topRight() + rect_.bottomRight()) / 2);
  path.arcTo(rect_, 0, 360 * 16);
  ShapePtr new_oval = std::make_shared<PathShape>(path);
  if(document().activeLayer()->type() == Layer::Type::Fill) new_oval->setFilled(true);
  document().execute(
       Commands::AddShape(document().activeLayer(), new_oval),
       Commands::Select(&document(), {new_oval})
  );
  Q_EMIT canvasUpdated();
  exit();
  return true;
}

void Oval::paint(QPainter *painter) {
  QPen pen(document().activeLayer()->color(), 2, Qt::SolidLine);
  pen.setCosmetic(true);
  painter->setPen(pen);
  painter->drawArc(rect_, 0, 360 * 16);
}

bool Oval::keyPressEvent(QKeyEvent *e) {
  setScaleLock(e->modifiers() & Qt::ShiftModifier);
  if (e->key() == Qt::Key::Key_Escape) {
    exit();
    return true;
  }
  return false;
}

bool Oval::keyReleaseEvent(QKeyEvent *e) {
  setScaleLock(e->modifiers() & Qt::ShiftModifier);
  return false;
}

void Oval::exit() {
  rect_ = QRectF(0, 0, 0, 0);
  canvas().setMode(Canvas::Mode::Selecting);
}

void Oval::setScaleLock(bool scale_lock) {
  aspect_ratio_ = scale_lock ? (mouse_pos_.y() - pressed_pos_.y()) / (mouse_pos_.x() - pressed_pos_.x()) : 1;
  scale_locked_ = scale_lock;
}
