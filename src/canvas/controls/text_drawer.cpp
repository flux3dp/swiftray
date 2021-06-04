#include <QApplication>
#include <QPainterPath>
#include <QDebug>
#include <shape/path_shape.h>
#include <canvas/controls/text_drawer.h>
#include <cmath>

bool TextDrawer::mousePressEvent(QMouseEvent *e) {
    if (scene().mode() != Scene::Mode::DRAWING_TEXT) return false;
    CanvasControl::mousePressEvent(e);
    return false;
}

bool TextDrawer::mouseMoveEvent(QMouseEvent *e) {
    if (scene().mode() != Scene::Mode::DRAWING_TEXT) return false;
    return true;
}

bool TextDrawer::mouseReleaseEvent(QMouseEvent *e) {
    if (scene().mode() != Scene::Mode::DRAWING_TEXT) return false;
    origin_ = scene().getCanvasCoord(e->pos());
    scene().text_box_->setFocus();
    if (target_ == nullptr) {
        // todo:: possible memory leak
        target_ = new TextShape("TEXT", QFont("Tahoma", 88, QFont::Bold));
        target_->setTransform(QTransform().translate(origin_.x(), origin_.y()));
    }
    return true;
}


bool TextDrawer::hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor) {
    if (scene().mode() != Scene::Mode::DRAWING_TEXT) return false;
    *cursor = Qt::IBeamCursor;
    return true;
}

bool TextDrawer::keyPressEvent(QKeyEvent *e) {
    if (scene().mode() != Scene::Mode::DRAWING_TEXT) return false;
    if (e->key() == Qt::Key::Key_Escape) {
        target_->editing_ = false;
        if (scene().text_box_->toPlainText().length() > 0) {
            ShapePtr new_shape(target_);
            scene().activeLayer().addShape(new_shape);
            scene().setSelection(new_shape);
        }
        target_ = nullptr;
        scene().setMode(Scene::Mode::SELECTING);
        return true;
    }
}

void TextDrawer::paint(QPainter *painter){
    if (scene().mode() != Scene::Mode::DRAWING_TEXT) return;
    if (target_ == nullptr) return;
    QString text = scene().text_box_->toPlainText() + scene().text_box_->preeditString();
    target_->setText(text);
    target_->makeCursorRect(scene().text_box_->textCursor().position());
    target_->editing_ = true;
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
}