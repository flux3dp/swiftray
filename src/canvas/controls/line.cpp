#include <QPainterPath>
#include <canvas/controls/line.h>
#include <cmath>
#include <shape/path-shape.h>

using namespace Controls;

bool Line::isActive() {
  return scene().mode() == Document::Mode::LineDrawing;
}

bool Line::mouseMoveEvent(QMouseEvent *e) {
  cursor_ = scene().getCanvasCoord(e->pos());
  return true;
}

bool Line::mouseReleaseEvent(QMouseEvent *e) {
  QPainterPath path;
  path.moveTo(scene().mousePressedCanvasCoord());
  path.lineTo(scene().getCanvasCoord(e->pos()));
  ShapePtr new_line = make_shared<PathShape>(path);
  scene().setMode(Document::Mode::Selecting);
  scene().execute(
       Commands::AddShape::shared(scene().activeLayer(), new_line) +
       Commands::Select::shared({new_line})
  );
  return true;
}

void Line::paint(QPainter *painter) {
  if (cursor_ == QPointF(0, 0))
    return;
  QPen pen(scene().activeLayer()->color(), 3, Qt::SolidLine);
  pen.setCosmetic(true);
  painter->setPen(pen);
  painter->drawLine(scene().mousePressedCanvasCoord(), cursor_);
}

void Line::reset() { cursor_ = QPointF(); }