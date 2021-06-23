#include <QPainterPath>
#include <cmath>
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
  path.lineTo(document().getCanvasCoord(e->pos()));
  ShapePtr new_line = make_shared<PathShape>(path);
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
  painter->drawLine(document().mousePressedCanvasCoord(), cursor_);
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