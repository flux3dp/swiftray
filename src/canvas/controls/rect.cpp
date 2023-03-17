#include <QPainterPath>
#include <canvas/controls/rect.h>
#include <shape/path-shape.h>
#include <canvas/canvas.h>

using namespace Controls;

bool Rect::isActive() {
  return canvas().mode() == Canvas::Mode::RectDrawing;
}

bool Rect::mouseMoveEvent(QMouseEvent *e) {
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

bool Rect::mouseReleaseEvent(QMouseEvent *e) {
  QPainterPath path;
  path.addRect(rect_);
  ShapePtr new_rect = std::make_shared<PathShape>(path);
  if(document().activeLayer()->type() == Layer::Type::Fill) new_rect->setFilled(true);
  canvas().setMode(Canvas::Mode::Selecting);
  document().execute(
       Commands::AddShape(document().activeLayer(), new_rect),
       Commands::Select(&document(), {new_rect})
  );
  Q_EMIT canvasUpdated();
  exit();
  return true;
}

void Rect::paint(QPainter *painter) {
  QPen pen(document().activeLayer()->color(), 2, Qt::SolidLine);
  pen.setCosmetic(true);
  painter->setPen(pen);
  painter->drawRect(rect_);
}


bool Rect::keyPressEvent(QKeyEvent *e) {
  setScaleLock(e->modifiers() & Qt::ShiftModifier);
  if (e->key() == Qt::Key::Key_Escape) {
    exit();
    return true;
  }
  return false;
}

bool Rect::keyReleaseEvent(QKeyEvent *e) {
  setScaleLock(e->modifiers() & Qt::ShiftModifier);
  return false;
}

void Rect::exit() {
  rect_ = QRectF(0, 0, 0, 0);
  canvas().setMode(Canvas::Mode::Selecting);
}

void Rect::setScaleLock(bool scale_lock) {
  aspect_ratio_ = scale_lock ? (mouse_pos_.y() - pressed_pos_.y()) / (mouse_pos_.x() - pressed_pos_.x()) : 1;
  scale_locked_ = scale_lock;
}
