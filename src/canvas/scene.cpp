#include <canvas/scene.hpp>
#include <QDebug>

Scene::Scene() noexcept {
    mode_ = Mode::SELECTING;
    pasting_shift = QPointF();
    new_layer_id_ = 1;
    addLayer();
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

void Scene::clearAll() {
    clearSelection();
    layers().clear();
    new_layer_id_ = 1;
    active_layer_ = nullptr;
    emit layerChanged();
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

void Scene::undo() {
    qInfo() << "Undo";

    if (undo_stack_.size() == 0) {
        return;
    }

    clearAll();
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
    emit layerChanged();
    undo_stack_.pop_back();
}


void Scene::redo() {
    qInfo() << "Redo";

    if (redo_stack_.size() == 0) {
        return;
    }

    clearAll();
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
    emit layerChanged();
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

void Scene::addLayer() {
    qDebug() << "Add layer";
    layers() << Layer();
    layers().last().name = "Layer " + QString::number(new_layer_id_++);
    active_layer_ = &layers().last();
    stackStep();
    emit layerChanged();
}

Scene::Mode Scene::mode() {
    return mode_;
}

void Scene::setMode(Mode mode) {
    mode_ = mode;
}

QPointF Scene::getCanvasCoord(QPointF window_coord) const {
    return (window_coord - QPointF(scroll_x, scroll_y)) / scale;
}
QPointF Scene::scroll() const {
    return QPointF(scroll_x, scroll_y);
}
Layer &Scene::activeLayer() {
    Q_ASSERT_X(layers_.size() != 0, "Active Layer", "Access to active layer when there is no layer");
    Q_ASSERT_X(active_layer_ != nullptr, "Active Layer", "Access to active layer is cleaned");
    return *active_layer_;
}

QList<Layer> &Scene::layers() {
    return layers_;
}
