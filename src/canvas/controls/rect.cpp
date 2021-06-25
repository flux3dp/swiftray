#include <QPainterPath>
#include <canvas/controls/rect.h>
#include <shape/path-shape.h>
#include <canvas/canvas.h>

using namespace Controls;

bool Rect::isActive() {
  return canvas().mode() == Canvas::Mode::RectDrawing;
}

bool Rect::mouseMoveEvent(QMouseEvent *e) {
  rect_ = QRectF(document().mousePressedCanvasCoord(), document().getCanvasCoord(e->pos()));
  return true;
}

bool Rect::mouseReleaseEvent(QMouseEvent *e) {
  QPainterPath path;
  path.addRect(rect_);
  ShapePtr new_rect = make_shared<PathShape>(path);
  canvas().setMode(Canvas::Mode::Selecting);
  document().execute(
       Commands::AddShape(document().activeLayer(), new_rect),
       Commands::Select(&document(), {new_rect})
  );
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
  if (e->key() == Qt::Key::Key_Escape) {
    exit();
    return true;
  }
  return false;
}

void Rect::exit() {
  rect_ = QRectF(0, 0, 0, 0);
  canvas().setMode(Canvas::Mode::Selecting);
}