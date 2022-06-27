#include <QPainterPath>
#include <canvas/controls/line.h>
#include <shape/path-shape.h>
#include <canvas/canvas.h>

using namespace Controls;

bool Line::isActive() {
  return canvas().mode() == Canvas::Mode::LineDrawing;
}

bool Line::mouseMoveEvent(QMouseEvent *e) {
  cursor_ = document().getCanvasCoord(e->pos());
  return true;
}

bool Line::mouseReleaseEvent(QMouseEvent *e) {
  QPainterPath path;
  path.moveTo(document().mousePressedCanvasCoord());
  QPointF new_point = document().getCanvasCoord(e->pos());
  if(direction_locked_) {
    QPointF move_point = new_point - document().mousePressedCanvasCoord();
    if(abs(move_point.x()) >= abs(move_point.y())) {
      new_point.setY(document().mousePressedCanvasCoord().y());
    }
    else {
      new_point.setX(document().mousePressedCanvasCoord().x());
    }
  }
  path.lineTo(new_point);
  ShapePtr new_line = std::make_shared<PathShape>(path);
  document().execute(
       Commands::AddShape(document().activeLayer(), new_line),
       Commands::Select(&document(), {new_line})
  );
  exit();
  return true;
}

void Line::paint(QPainter *painter) {
  if (cursor_ == QPointF(0, 0))
    return;
  QPen pen(document().activeLayer()->color(), 3, Qt::SolidLine);
  pen.setCosmetic(true);
  painter->setPen(pen);
  if(direction_locked_) {
    QPointF new_point = cursor_;
    QPointF move_point = cursor_ - document().mousePressedCanvasCoord();
    if(abs(move_point.x()) >= abs(move_point.y())) {
      new_point.setY(document().mousePressedCanvasCoord().y());
    }
    else {
      new_point.setX(document().mousePressedCanvasCoord().x());
    }
    painter->drawLine(document().mousePressedCanvasCoord(), new_point);
  }
  else {
    painter->drawLine(document().mousePressedCanvasCoord(), cursor_);
  }
}

bool Line::keyPressEvent(QKeyEvent *e) {
  if (e->key() == Qt::Key::Key_Escape) {
    exit();
    return true;
  }
  return false;
}

void Line::exit() {
  cursor_ = QPointF();
  canvas().setMode(Canvas::Mode::Selecting);
}

bool Line::isDirectionLock() const {
  return direction_locked_;
}

void Line::setDirectionLock(bool direction_lock) {
  direction_locked_ = direction_lock;
}