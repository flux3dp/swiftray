#include <canvas/canvas_data.hpp>
#include <QDebug>

void CanvasData::setSelection(ShapePtr shape) {
    QList<ShapePtr> list;
    list.push_back(shape);
    setSelections(list);
}
void CanvasData::setSelections(QList<ShapePtr> &shapes) {
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

bool CanvasData::isSelected(ShapePtr shape) {
    return selections().contains(shape);
}

QList<ShapePtr> &CanvasData::selections() {
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

    QList<ShapePtr> cloned;

    for (int i = 0 ; i < shapes_.length(); i++) {
        cloned.push_back(shapes_[i]->clone());
    }

    undo_stack.push_back(cloned);
}

// TODO: fix gc and deconstructor
void CanvasData::undo() {
    qInfo() << "Undo";

    if (undo_stack.size() == 0) {
        return;
    }

    clear();
    shapes_.append(undo_stack.last());
    redo_stack.push_back(undo_stack.last());
    QList<shared_ptr<Shape>> selected;

    for (int i = 0; i < shapes_.length(); i++) {
        if (shapes_.at(i)->selected) {
            selected << shapes_.at(i);
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
    QList<ShapePtr> selected;

    for (int i = 0; i < shapes_.length(); i++) {
        if (shapes_.at(i)->selected) {
            selected << shapes_.at(i);
        }
    }

    setSelections(selected);
    redo_stack.pop_back();
}

ShapeCollection &CanvasData::shapes() {
    return shapes_;
}
