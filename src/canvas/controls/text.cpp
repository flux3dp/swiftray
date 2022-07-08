#include <QApplication>
#include <QDebug>
#include <QPainterPath>

#include <canvas/controls/text.h>
#include <cmath>
#include <shape/path-shape.h>
#include <canvas/canvas.h>

using namespace Controls;

Text::Text(Canvas *canvas) noexcept: CanvasControl(canvas) {
  connect(canvas, &Canvas::undoCalled, this, &Text::exit);
}

bool Text::isActive() {
  return canvas().mode() == Canvas::Mode::TextDrawing;
}

bool Text::mouseReleaseEvent(QMouseEvent *e) {
  QPointF canvas_coord = document().getCanvasCoord(e->pos());
  canvas().textInput()->setFocus();
  if (target_ == nullptr) {
    // Create a virtual target
    ShapePtr new_shape = std::make_shared<TextShape>("", canvas().font(), canvas().lineHeight());
    setTarget(new_shape);
    target().setTransform(QTransform().translate(canvas_coord.x(), canvas_coord.y()));
  }
  return true;
}

bool Text::hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor) {
  *cursor = Qt::IBeamCursor;
  return true;
}

bool Text::keyPressEvent(QKeyEvent *e) {
  if (e->key() == Qt::Key::Key_Escape) {
    exit();
    return true;
  }
  return false;
}

void Text::paint(QPainter *painter) {
  if (target_ == nullptr)
    return;
  QString text = canvas().textInput()->toPlainText() + canvas().textInput()->preeditString();
  if (text_cache_ != text) {
    target().setText(text);
    text_cache_ = text;
  }
  target().makeCursorRect(canvas().textInput()->textCursor().position());
  target().setEditing(true);
  QPen pen(document().activeLayer()->color(), 2, Qt::SolidLine);
  pen.setCosmetic(true);
  painter->setPen(pen);
  target_->paint(painter);
}

void Text::exit() {
  if (target_ != nullptr) {
    target().setEditing(false);
    if (!target().hasLayer() &&
        canvas().textInput()->toPlainText().length() > 0) {
      // Add the virtual target the layer
      document().execute(
           Commands::AddShape(document().activeLayer(), target_),
           Commands::Select(&document(), {target_})
      );
    }
  }
  target_ = nullptr;
  canvas().textInput()->clear();
  canvas().textInput()->window()->setFocus();
  emit canvas().selectionsChanged();
  canvas().setMode(Canvas::Mode::Selecting);
}

TextShape &Text::target() {
  return static_cast<TextShape &>(*target_.get());
}

void Text::setTarget(ShapePtr &new_target) {
  target_ = new_target;
  if (target_ != nullptr) {
    canvas().textInput()->setPlainText(target().text());
  }
}

bool Text::isEmpty() {
  if(target_ != nullptr) {
    return false;
  }
  else {
    return true;
  }
}