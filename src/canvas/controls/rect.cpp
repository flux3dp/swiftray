#include <QPainterPath>
#include <canvas/controls/rect.h>
#include <shape/path_shape.h>

using namespace Controls;

bool Rect::isActive() {
  return scene().mode() == Scene::Mode::RectDrawing;
}

bool Rect::mouseMoveEvent(QMouseEvent *e) {
  rect_ = QRectF(scene().mousePressedCanvasCoord(), scene().getCanvasCoord(e->pos()));
  return true;
}

bool Rect::mouseReleaseEvent(QMouseEvent *e) {
  QPainterPath path;
  path.addRect(rect_);
  ShapePtr new_rect = make_shared<PathShape>(path);
  scene().stackStep();
  scene().activeLayer()->addShape(new_rect);
  scene().setSelection(new_rect);
  scene().setMode(Scene::Mode::Selecting);
  return true;
}

void Rect::paint(QPainter *painter) {
  QPen pen(scene().activeLayer()->color(), 3, Qt::SolidLine);
  pen.setCosmetic(true);
  painter->setPen(pen);
  painter->drawRect(rect_);
}

void Rect::reset() { rect_ = QRectF(0, 0, 0, 0); }