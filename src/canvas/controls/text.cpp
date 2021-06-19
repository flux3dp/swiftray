#include <QApplication>
#include <QDebug>
#include <QPainterPath>

#include <canvas/controls/text.h>
#include <cmath>
#include <shape/path-shape.h>

using namespace Controls;

bool Text::isActive() {
  return document().mode() == Document::Mode::TextDrawing;
}

bool Text::mouseReleaseEvent(QMouseEvent *e) {
  origin_ = document().getCanvasCoord(e->pos());
  document().text_box_->setFocus();
  if (target_ == nullptr) {
    qInfo() << "Create virtual text shape";
    ShapePtr new_shape = make_shared<TextShape>("", document().font());
    setTarget(new_shape);
    target().setTransform(QTransform().translate(origin_.x(), origin_.y()));
  }
  return true;
}

bool Text::hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor) {
  *cursor = Qt::IBeamCursor;
  return true;
}

bool Text::keyPressEvent(QKeyEvent *e) {
  if (e->key() == Qt::Key::Key_Escape) {
    if (target_ == nullptr) return false;
    target().setEditing(false);
    if (!target().hasLayer() &&
        document().text_box_->toPlainText().length() > 0) {
      qInfo() << "Create new text shape instance";
      document().execute(
           Commands::AddShape::shared(document().activeLayer(), target_) +
           Commands::Select::shared(&document(), {target_})
      );
    } else {
      document().execute(Commands::Select::shared(&document(), {target_}));
    }
    exit();
    return true;
  }
  return false;
}

void Text::paint(QPainter *painter) {
  if (target_ == nullptr)
    return;
  QString text =
       document().text_box_->toPlainText() + document().text_box_->preeditString();
  target().setText(text);
  target().makeCursorRect(document().text_box_->textCursor().position());
  target().setEditing(true);
  QPen pen(document().activeLayer()->color(), 2, Qt::SolidLine);
  pen.setCosmetic(true);
  painter->setPen(pen);
  target_->paint(painter);
}

void Text::exit() {
  blink_counter = 0;
  target_ = nullptr;
  origin_ = QPointF();
  document().text_box_->clear();
  document().setMode(Document::Mode::Selecting);
}

TextShape &Text::target() {
  return static_cast<TextShape &>(*target_.get());
}

bool Text::hasTarget() {
  return target_ != nullptr;
}

void Text::setTarget(ShapePtr &new_target) {
  target_ = new_target;
  if (target_ != nullptr) {
    document().text_box_->setPlainText(target().text());
  }
}