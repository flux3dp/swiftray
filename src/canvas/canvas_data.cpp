#include <canvas/scene.h>
#include <QDebug>

Scene::Scene() noexcept {
    mode_ = Mode::SELECTING;
    layers_ << Layer();
    pasting_shift = QPointF();
}

void Scene::setSelection(ShapePtr shape) {
    QList<ShapePtr> list;
    list.push_back(shape);
    setSelections(list);
}
void Scene::setSelections(QList<ShapePtr> &shapes) {
    clearSelection();
    selections().append(shapes);

    for (int i = 0; i < selections().size(); i++) {
        selections().at(i)->selected = true;
    }

    emit selectionsChanged();
}

void Scene::clearSelection() {
    for (int i = 0; i < selections().size(); i++) {
        selections().at(i)->selected = false;
    }

    selections().clear();
    emit selectionsChanged();
}

void Scene::clearSelectionNoFlag() {
    selections().clear();
    emit selectionsChanged();
}

bool Scene::isSelected(ShapePtr shape) {
    return selections().contains(shape);
}

QList<ShapePtr> &Scene::selections() {
    return selections_;
}

void Scene::clear() {
    clearSelection();
    layers().clear();
}

void Scene::stackStep() {
    qInfo() << "<<<Stack>>";

    if (undo_stack_.size() > 19) {
        undo_stack_.pop_front();
    }

    QList<Layer> cloned;

    for (Layer &layer : layers()) {
        cloned.push_back(layer.clone());
    }

    undo_stack_.push_back(cloned);
}

// TODO: fix gc and deconstructor
void Scene::undo() {
    qInfo() << "Undo";

    if (undo_stack_.size() == 0) {
        return;
    }

    clear();
    layers().append(undo_stack_.last());
    // redo stack doesn't clone the pointers... should be fixed
    redo_stack_.push_back(undo_stack_.last());
    QList<ShapePtr> selected;

    for (Layer &layer : layers()) {
        for (ShapePtr &shape : layer.children()) {
            if (shape->selected) selected << shape;
        }
    }

    setSelections(selected);
    undo_stack_.pop_back();
}


void Scene::redo() {
    qInfo() << "Redo";

    if (redo_stack_.size() == 0) {
        return;
    }

    clear();
    // redo stack doesn't
    layers().append(undo_stack_.last());
    // undoo stack doesn't clone the pointers... should be fixed
    undo_stack_.push_back(redo_stack_.last());
    QList<ShapePtr> selected;

    for (Layer &layer : layers()) {
        for (ShapePtr &shape : layer.children()) {
            if (shape->selected) selected << shape;
        }
    }

    setSelections(selected);
    redo_stack_.pop_back();
}

QList<ShapePtr> &Scene::clipboard() {
    return shape_clipboard_;
}


void Scene::setClipboard(QList<ShapePtr> &items) {
    for (ShapePtr &item : items) {
        shape_clipboard_.push_back(item->clone());
    }
}
