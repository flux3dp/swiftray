#include <QPainterPath>

#include <canvas/controls/oval.h>
#include <cmath>
#include <shape/path-shape.h>

using namespace Controls;

bool Oval::isActive() {
  return document().mode() == Document::Mode::OvalDrawing;
}

bool Oval::mouseMoveEvent(QMouseEvent *e) {
  rect_ = QRectF(document().mousePressedCanvasCoord(), document().getCanvasCoord(e->pos()));
  return true;
}

bool Oval::mouseReleaseEvent(QMouseEvent *e) {
  QPainterPath path;
  path.moveTo((rect_.topRight() + rect_.bottomRight()) / 2);
  path.arcTo(rect_, 0, 360 * 16);
  ShapePtr new_oval = make_shared<PathShape>(path);
  document().execute(
       Commands::AddShape::shared(document().activeLayer(), new_oval) +
       Commands::Select::shared(&document(), {new_oval})
  );
  exit();
  return true;
}

void Oval::paint(QPainter *painter) {
  QPen pen(document().activeLayer()->color(), 3, Qt::SolidLine);
  pen.setCosmetic(true);
  painter->setPen(pen);
  painter->drawArc(rect_, 0, 360 * 16);
}

bool Oval::keyPressEvent(QKeyEvent *e) {
  if (e->key() == Qt::Key::Key_Escape) {
    exit();
    return true;
  }
  return false;
}

void Oval::exit() {
  rect_ = QRectF(0, 0, 0, 0);
  document().setMode(Document::Mode::Selecting);
}