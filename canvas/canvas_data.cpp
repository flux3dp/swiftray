#include <canvas/canvas_data.hpp>
#include <QDebug>

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

void CanvasData::clear() {
    clearSelection();
    shapes_.clear();
}

void CanvasData::stackStep() {
    qInfo() << "<<<Stack>>";

    if (undo_stack.size() > 19) {
        undo_stack.pop_front();
    }

    QList<Shape> clone;
    std::copy(shapes_.begin(), shapes_.end(), std::back_inserter(clone));
    undo_stack.push_back(clone);
}

void CanvasData::undo() {
    qInfo() << "Undo";

    if (undo_stack.size() == 0) {
        return;
    }

    clear();
    shapes_.append(undo_stack.last());
    redo_stack.push_back(undo_stack.last());
    QList<Shape *> selected;

    for (int i = 0; i < shapes_.length(); i++) {
        if (shapes_[i].selected) {
            selected << &shapes_[i];
        }
    }

    setSelections(selected);
    undo_stack.pop_back();
}


void CanvasData::redo() {
    qInfo() << "Redo";

    if (redo_stack.size() == 0) {
        return;
    }

    clear();
    shapes_.append(redo_stack.last());
    undo_stack.push_back(redo_stack.last());
    QList<Shape *> selected;

    for (int i = 0; i < shapes_.length(); i++) {
        if (shapes_[i].selected) {
            selected << &shapes_[i];
        }
    }

    setSelections(selected);
    redo_stack.pop_back();
}

Shape *CanvasData::shapesAt(int i) {
    return &shapes_[i];
}
