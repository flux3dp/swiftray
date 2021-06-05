#include <QDebug>
#include <canvas/controls/multi_selection_box.h>

bool MultiSelectionBox::mousePressEvent(QMouseEvent *e) {
    CanvasControl::mousePressEvent(e);
    selection_box_ = QRectF(0, 0, 0, 0);
    return false;
}

bool MultiSelectionBox::mouseMoveEvent(QMouseEvent *e) {
    if (scene().mode() != Scene::Mode::MULTI_SELECTING) return false;
    QPointF canvas_coord = scene().getCanvasCoord(e->pos());
    selection_box_ = QRectF(
                        min(dragged_from_canvas_.x(), canvas_coord.x()),
                        min(dragged_from_canvas_.y(), canvas_coord.y()),
                        abs(dragged_from_canvas_.x() - canvas_coord.x()),
                        abs(dragged_from_canvas_.y() - canvas_coord.y()));
    return true;
}

bool MultiSelectionBox::mouseReleaseEvent(QMouseEvent *e) {
    if (scene().mode() != Scene::Mode::MULTI_SELECTING) return false;
    if (selection_box_.width() != 0 || selection_box_.height() != 0) {
        QList<ShapePtr> selected;
        for (auto &layer : scene().layers()) {
            for (auto &shape : layer->children()) {
                if (shape->testHit(selection_box_)) {
                    selected << shape;
                }
            }
        }
        scene().setSelections(selected);
    }

    scene().setMode(Scene::Mode::SELECTING);
    return true;
}

void MultiSelectionBox::paint(QPainter *painter) {
    if (scene().mode() != Scene::Mode::MULTI_SELECTING) return;

    painter->setPen(QPen(QColor::fromRgb(0x00, 0x99, 0xCC, 255), 0, Qt::DashLine));
    painter->fillRect(selection_box_, QBrush(QColor::fromRgb(0x00, 0x99, 0xCC, 30)));
    painter->drawRect(selection_box_);
}