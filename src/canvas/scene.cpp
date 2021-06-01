#include <canvas/scene.h>
#include <QDebug>

Scene::Scene() noexcept {
    mode_ = Mode::SELECTING;
    pasting_shift = QPointF();
    new_layer_id_ = 1;
    scroll_x_ = 0;
    scroll_y_ = 0;
    scale_ = 1;
    addLayer();
}

void Scene::setSelection(ShapePtr shape) {
    QList<ShapePtr> list;
    list.push_back(shape);
    setSelections(list);
}

void Scene::setSelections(QList<ShapePtr> &shapes) {
    clearSelections();
    selections().append(shapes);

    for (int i = 0; i < selections().size(); i++) {
        selections().at(i)->selected = true;
    }

    emit selectionsChanged();
}

void Scene::clearSelections() {
    for (int i = 0; i < selections().size(); i++) {
        selections().at(i)->selected = false;
    }

    selections().clear();
    emit selectionsChanged();
}

void Scene::clearSelectionsNoFlag() {
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
    clearSelections();
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
    return (window_coord - scroll()) / scale();
}

QPointF Scene::scroll() const {
    return QPointF(scroll_x_, scroll_y_);
}


qreal Scene::scrollX() const {
    return scroll_x_;
}

qreal Scene::scrollY() const {
    return scroll_y_;
}

qreal Scene::scale() const {
    return scale_;
}


void Scene::setScroll(QPointF scroll) {
    scroll_x_ = scroll.x();
    scroll_y_ = scroll.y();
}

void Scene::setScrollX(qreal scroll_x) {
    scroll_x_ = scroll_x;

}

void Scene::setScrollY(qreal scroll_y) {
    scroll_y_ = scroll_y;

}

void Scene::setScale(qreal scale) {
    scale_ = scale;
}

Layer &Scene::activeLayer() {
    Q_ASSERT_X(layers_.size() != 0, "Active Layer", "Access to active layer when there is no layer");
    Q_ASSERT_X(active_layer_ != nullptr, "Active Layer", "Access to active layer is cleaned");
    return *active_layer_;
}

QList<Layer> &Scene::layers() {
    return layers_;
}

void Scene::removeSelections() {
    // Need to clean up all selection pointer reference first
    clearSelectionsNoFlag();

    for (Layer &layer : layers()) {
        layer.children().erase(std::remove_if(layer.children().begin(), layer.children().end(), [](ShapePtr & s) {
            return s->selected;
        }), layer.children().end());
    }
}

ShapePtr Scene::hitTest(QPointF canvas_coord) {
    for (Layer &layer : layers()) {
        for (ShapePtr &shape : layer.children()) {
            if (shape->testHit(canvas_coord, 10 / scale())) {
                return shape;
            }
        }
    }
    return nullptr;
}