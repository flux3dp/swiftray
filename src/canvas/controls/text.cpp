#include <QApplication>
#include <QDebug>
#include <QPainterPath>
#include <canvas/controls/text.h>
#include <cmath>
#include <shape/path-shape.h>

using namespace Controls;

bool Text::isActive() {
  return scene().mode() == Document::Mode::TextDrawing;
}

bool Text::mouseReleaseEvent(QMouseEvent *e) {
  origin_ = scene().getCanvasCoord(e->pos());
  scene().text_box_->setFocus();
  if (target_ == nullptr) {
    qInfo() << "Create virtual text shape";
    ShapePtr new_shape = make_shared<TextShape>("", scene().font());
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
        scene().text_box_->toPlainText().length() > 0) {
      qInfo() << "Create new text shape instance";
      scene().execute(
           Commands::AddShape::shared(scene().activeLayer(), target_) +
           Commands::Select::shared({target_})
      );
    } else {
      scene().execute(Commands::Select::shared({target_}));
    }
    reset();
    scene().setMode(Document::Mode::Selecting);
    return true;
  }
  return false;
}

void Text::paint(QPainter *painter) {
  if (target_ == nullptr)
    return;
  QString text =
       scene().text_box_->toPlainText() + scene().text_box_->preeditString();
  target().setText(text);
  target().makeCursorRect(scene().text_box_->textCursor().position());
  target().setEditing(true);
  QPen pen(scene().activeLayer()->color(), 2, Qt::SolidLine);
  pen.setCosmetic(true);

  QPen caret_pen(Qt::black, 2, Qt::SolidLine);
  caret_pen.setCosmetic(true);

  painter->setPen(pen);
  target_->paint(painter);
  painter->setPen(caret_pen);
}

void Text::reset() {
  blink_counter = 0;
  target_ = nullptr;
  origin_ = QPointF();
  scene().text_box_->clear();
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
    scene().text_box_->setPlainText(target().text());
  }
}