#include <QPainterPath>
#include <canvas/controls/rect.h>
#include <shape/path-shape.h>

using namespace Controls;

bool Rect::isActive() {
  return document().mode() == Document::Mode::RectDrawing;
}

bool Rect::mouseMoveEvent(QMouseEvent *e) {
  rect_ = QRectF(document().mousePressedCanvasCoord(), document().getCanvasCoord(e->pos()));
  return true;
}

bool Rect::mouseReleaseEvent(QMouseEvent *e) {
  QPainterPath path;
  path.addRect(rect_);
  ShapePtr new_rect = make_shared<PathShape>(path);
  document().setMode(Document::Mode::Selecting);
  document().execute(
       Commands::AddShape(document().activeLayer(), new_rect),
       Commands::Select(&document(), {new_rect})
  );
  return true;
}

void Rect::paint(QPainter *painter) {
  QPen pen(document().activeLayer()->color(), 3, Qt::SolidLine);
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
  document().setMode(Document::Mode::Selecting);
}