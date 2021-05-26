#include <canvas/canvas_data.hpp>

void CanvasData::setSelection(Shape *shape) {
    QList<Shape *> list;
    list << shape;
    setSelections(list);
}
void CanvasData::setSelections(QList<Shape *> &shapes) {
    clearSelection();
    selections().append(shapes);

    for (int i = 0; i < selections().size(); i++) {
        selections().at(i)->selected = true;
    }

    emit selectionsChanged();
}

void CanvasData::clearSelection() {
    for (int i = 0; i < selections().size(); i++) {
        selections().at(i)->selected = false;
    }

    selections().clear();
    emit selectionsChanged();
}

void CanvasData::clearSelectionNoFlag() {
    selections().clear();
    emit selectionsChanged();
}

bool CanvasData::isSelected(Shape *shape) {
    return selections().contains(shape);
}
QList<Shape *> &CanvasData::selections() {
    return selections_;
}
