#include <QApplication>
#include <QDebug>
#include <QPainterPath>
#include <canvas/controls/text_drawer.h>
#include <cmath>
#include <shape/path_shape.h>

bool TextDrawer::isActive() { 
    return scene().mode() == Scene::Mode::DRAWING_TEXT; 
}

bool TextDrawer::mouseReleaseEvent(QMouseEvent *e) {
    origin_ = scene().getCanvasCoord(e->pos());
    scene().text_box_->setFocus();
    if (target_ == nullptr) {
        qInfo() << "Create virtual text shape";
        setTarget(make_shared<TextShape>("", QFont("Tahoma", 88, QFont::Bold)));
        target().setTransform(QTransform().translate(origin_.x(), origin_.y()));
    }
    return true;
}

bool TextDrawer::hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor) {
    if (scene().mode() != Scene::Mode::DRAWING_TEXT)
        return false;
    *cursor = Qt::IBeamCursor;
    return true;
}

bool TextDrawer::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key::Key_Escape) {
        target().setEditing(false);
        if (target().parent() == nullptr &&
            scene().text_box_->toPlainText().length() > 0) {
            qInfo() << "Create new text shape instance";
            scene().stackStep();
            scene().activeLayer().addShape(target_);
            scene().setSelection(target_);
        }
        setTarget(nullptr);
        scene().setMode(Scene::Mode::SELECTING);
        return true;
    }
    return false;
}

void TextDrawer::paint(QPainter *painter) {
    if (target_ == nullptr)
        return;
    QString text =
        scene().text_box_->toPlainText() + scene().text_box_->preeditString();
    target().setText(text);
    target().makeCursorRect(scene().text_box_->textCursor().position());
    target().setEditing(true);
    QPen pen(scene().activeLayer().color(), 3, Qt::SolidLine);
    pen.setCosmetic(true);

    QPen caret_pen(Qt::black, 5, Qt::SolidLine);
    caret_pen.setCosmetic(true);

    painter->setPen(pen);
    target_->paint(painter);
    painter->setPen(caret_pen);
}

void TextDrawer::reset() {
    blink_counter = 0;
    target_ = nullptr;
    origin_ = QPointF();
    scene().text_box_->clear();
}

TextShape &TextDrawer::target() {
    return *dynamic_cast<TextShape *>(target_.get());
}

void TextDrawer::setTarget(ShapePtr new_target) {
    target_ = new_target;
    if (target_ != nullptr) {
        scene().text_box_->setPlainText(target().text());
    }
}