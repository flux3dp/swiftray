#include <QDebug>
#include <canvas/controls/multi_selection_box.h>

MultiSelectionBox::MultiSelectionBox(Scene &scene_) noexcept : CanvasControl(scene_) {
    selection_box_ = QRectF(0, 0, 0, 0);
}

bool MultiSelectionBox::isActive() { 
    return scene().mode() == Scene::Mode::MULTI_SELECTING; 
}

bool MultiSelectionBox::mouseMoveEvent(QMouseEvent *e) {
    QPointF start = scene().mousePressedCanvasCoord();
    QPointF canvas_coord = scene().getCanvasCoord(e->pos());
    selection_box_ = QRectF(min(start.x(), canvas_coord.x()),
                            min(start.y(), canvas_coord.y()),
                            abs(start.x() - canvas_coord.x()),
                            abs(start.y() - canvas_coord.y()));
    return true;
}

bool MultiSelectionBox::mouseReleaseEvent(QMouseEvent *e) {
    if (selection_box_.width() != 0 || selection_box_.height() != 0) {
        QList<ShapePtr> selected;
        for (auto &layer : scene().layers()) {
            for (auto &shape : layer->children()) {
                if (shape->hitTest(selection_box_)) {
                    selected << shape;
                }
            }
        }
        scene().setSelections(selected);
    }

    scene().setMode(Scene::Mode::SELECTING);
    selection_box_ = QRectF(0, 0, 0, 0);
    return true;
}

void MultiSelectionBox::paint(QPainter *painter) {
    painter->setPen(
        QPen(QColor::fromRgb(0x00, 0x99, 0xCC, 255), 0, Qt::DashLine));
    painter->fillRect(selection_box_,
                      QBrush(QColor::fromRgb(0x00, 0x99, 0xCC, 30)));
    painter->drawRect(selection_box_);
}