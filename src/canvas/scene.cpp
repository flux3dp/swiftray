#include <canvas/scene.h>
#include <QDebug>

Scene::Scene() noexcept {
    mode_ = Mode::SELECTING;
    pasting_shift = QPointF();
    new_layer_id_ = 1;
    scroll_x_ = 0;
    scroll_y_ = 0;
    scale_ = 1;
    width_ = 3000;
    height_ = 2000;
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
    redo_stack_.clear();
    stackUndo();
}

void Scene::stackUndo() {
    if (undo_stack_.size() > 19) {
        undo_stack_.pop_front();
    }
    undo_stack_.push_back(cloneStack(layers_));
}

void Scene::stackRedo() {
    if (redo_stack_.size() > 19) {
        redo_stack_.pop_front();
    }
    redo_stack_.push_back(cloneStack(layers_));
}

QList<LayerPtr> Scene::cloneStack(QList<LayerPtr> &stack) {
    QList<LayerPtr> cloned;

    for (auto &layer : stack) {
        cloned << layer->clone();
    }

    return cloned;
}

void Scene::dumpStack(QList<LayerPtr> &stack) {
    qInfo() << "<Stack>";
    for (auto &layer : stack) {
        qInfo() << "  <Layer " << &layer << ">";
        for (auto &shape : layer->children()) {
            qInfo() << "    <Shape "<< shape.get() << " selected =" << shape->selected << " />";
        }
        qInfo() << "  </Layer>";
    }
    qInfo() << "</Stack>";
}

void Scene::undo() {
    if (undo_stack_.isEmpty()) {
        return;
    }
    QString active_layer_name = activeLayer().name;
    stackRedo();
    clearAll();
    Scene::dumpStack(undo_stack_.last());
    layers().append(undo_stack_.last());
    QList<ShapePtr> selected;

    for (auto &layer : layers()) {
        for (auto &shape : layer->children()) {
            if (shape->selected) selected << shape;
        }
    }

    setSelections(selected);
    setActiveLayer(active_layer_name);
    emit layerChanged();
    undo_stack_.pop_back();
}


void Scene::redo() {
    if (redo_stack_.isEmpty()) {
        return;
    }
    QString active_layer_name = activeLayer().name;
    stackUndo();
    clearAll();
    layers().append(redo_stack_.last());
    QList<ShapePtr> selected;

    for (auto &layer : layers()) {
        for (auto &shape : layer->children()) {
            if (shape->selected) selected << shape;
        }
    }
    
    setSelections(selected);
    setActiveLayer(active_layer_name);
    emit layerChanged();
    redo_stack_.pop_back();
}

QList<ShapePtr> &Scene::clipboard() {
    return shape_clipboard_;
}


void Scene::setClipboard(QList<ShapePtr> &items) {
    for (auto &item : items) {
        shape_clipboard_.push_back(item->clone());
    }
}

void Scene::addLayer() {
    if (layers().length() > 0) stackStep();
    qDebug() << "Add layer";
    layers() << make_shared<Layer>();
    layers().last()->name = "Layer " + QString::number(new_layer_id_++);
    active_layer_ = layers().last();
    emit layerChanged();
}

Scene::Mode Scene::mode() {
    return mode_;
}

void Scene::setMode(Mode mode) {
    mode_ = mode;
    emit modeChanged();
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

qreal Scene::width() const{
    return width_;
}

qreal Scene::height() const {
    return height_;
}


void Scene::setWidth(qreal width) {
    width_ = width;
}

void Scene::setHeight(qreal height) {
    height_ = height;

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

bool Scene::setActiveLayer(QString name) {
    for (auto &layer : layers()) {
        if (layer->name == name) {
            active_layer_ = layer;
            return true;
        }
    }

    active_layer_ = layers().last();
    return false;
}


QList<LayerPtr> &Scene::layers() {
    return layers_;
}

void Scene::removeSelections() {
    // Clear selection pointers in other componenets
    selections().clear();
    emit selectionsChanged();

    // Remove
    for (auto &layer : layers()) {
        layer->children().erase(std::remove_if(layer->children().begin(), layer->children().end(), [](ShapePtr & s) {
            return s->selected;
        }), layer->children().end());
    }
}

ShapePtr Scene::hitTest(QPointF canvas_coord) {
    for (auto &layer : layers()) {
        for (auto &shape : layer->children()) {
            if (shape->hitTest(canvas_coord, 5 / scale())) {
                return shape;
            }
        }
    }
    return nullptr;
}