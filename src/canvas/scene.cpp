#include <QDebug>
#include <boost/range/adaptor/reversed.hpp>
#include <canvas/scene.h>

Scene::Scene() noexcept {
  mode_ = Mode::Selecting;
  new_layer_id_ = 1;
  scroll_x_ = 0;
  scroll_y_ = 0;
  scale_ = 1;
  width_ = 3000;
  height_ = 2000;
  font_ = QFont("Tahoma", 200, QFont::Bold);
  addLayer();
  connect(this, &Scene::selectionsChanged, [=]() {
    for (auto &layer : layers_) {
      layer->flushCache();
    }
  });
}

void Scene::setSelection(ShapePtr &shape) {
  QList<ShapePtr> list;
  list.push_back(shape);
  setSelections(list);
}

void Scene::setSelections(const QList<ShapePtr> &shapes) {
  clearSelections();
  selections().append(shapes);

  for (auto &shape : selections()) {
    shape->setSelected(true);
  }
  emit selectionsChanged();
}

void Scene::clearSelections() {
  for (auto &shape : selections()) {
    shape->setSelected(false);
  }

  selections().clear();
  emit selectionsChanged();
}

bool Scene::isSelected(ShapePtr &shape) const { return selections_.contains(shape); }

QList<ShapePtr> &Scene::selections() { return selections_; }

void Scene::clearAll() {
  clearSelections();
  layers().clear();
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
    qInfo() << "  <Layer " << layer.get() << ">";
    for (auto &shape : layer->children()) {
      qInfo() << "    <Shape " << shape.get()
              << " selected =" << shape->selected() << " />";
    }
    qInfo() << "  </Layer>";
  }
  qInfo() << "</Stack>";
}

void Scene::undo() {
  if (undo_stack_.isEmpty()) {
    return;
  }
  QString active_layer_name = activeLayer()->name();
  stackRedo();
  clearSelections();
  layers().clear();
  Scene::dumpStack(undo_stack_.last());
  layers().append(undo_stack_.last());
  QList<ShapePtr> selected_shapes;

  for (auto &layer : layers()) {
    for (auto &shape : layer->children()) {
      if (shape->selected())
        selected_shapes << shape;
    }
  }

  setSelections(selected_shapes);
  setActiveLayer(active_layer_name);
  emit layerChanged();
  undo_stack_.pop_back();
}

void Scene::redo() {
  if (redo_stack_.isEmpty()) {
    return;
  }
  QString active_layer_name = activeLayer()->name();
  stackUndo();
  clearSelections();
  layers().clear();
  layers().append(redo_stack_.last());
  QList<ShapePtr> selected_shapes;

  for (auto &layer : layers()) {
    for (auto &shape : layer->children()) {
      if (shape->selected())
        selected_shapes << shape;
    }
  }

  setSelections(selected_shapes);
  setActiveLayer(active_layer_name);
  emit layerChanged();
  redo_stack_.pop_back();
}

const QList<ShapePtr> &Scene::clipboard() const { return shape_clipboard_; }

void Scene::clearClipboard() { shape_clipboard_.clear(); }

void Scene::setClipboard(QList<ShapePtr> &items) {
  for (auto &item : items) {
    shape_clipboard_.push_back(item->clone());
  }
}

void Scene::addLayer() {
  if (layers().length() > 0)
    stackStep();
  qDebug() << "Add layer";
  layers() << make_shared<Layer>(new_layer_id_++);
  active_layer_ = layers().last();
  emit layerChanged();
}

void Scene::addLayer(LayerPtr &layer) {
  qDebug() << "Add layer by ptr" << layer->name();
  layers() << layer->clone();
  active_layer_ = layers().last();
}

void Scene::emitAllChanges() {
  emit selectionsChanged();
  emit layerChanged();
  emit modeChanged();
}

Scene::Mode Scene::mode() const { return mode_; }

void Scene::setMode(Mode mode) {
  mode_ = mode;
  emit modeChanged();
}

QPointF Scene::getCanvasCoord(QPointF window_coord) const {
  return (window_coord - scroll()) / scale();
}

QPointF Scene::scroll() const { return QPointF(scroll_x_, scroll_y_); }

qreal Scene::scrollX() const { return scroll_x_; }

qreal Scene::scrollY() const { return scroll_y_; }

qreal Scene::scale() const { return scale_; }

qreal Scene::width() const { return width_; }

qreal Scene::height() const { return height_; }

void Scene::setWidth(qreal width) { width_ = width; }

void Scene::setHeight(qreal height) { height_ = height; }

void Scene::setScroll(QPointF scroll) {
  scroll_x_ = scroll.x();
  scroll_y_ = scroll.y();
}

void Scene::setScrollX(qreal scroll_x) { scroll_x_ = scroll_x; }

void Scene::setScrollY(qreal scroll_y) { scroll_y_ = scroll_y; }

void Scene::setScale(qreal scale) { scale_ = scale; }

LayerPtr &Scene::activeLayer() {
  Q_ASSERT_X(layers_.size() != 0, "Active Layer",
             "Access to active layer when there is no layer");
  Q_ASSERT_X(active_layer_ != nullptr, "Active Layer",
             "Access to active layer is cleaned");
  return active_layer_;
}

bool Scene::setActiveLayer(QString name) {
  for (auto &layer : layers()) {
    if (layer->name() == name) {
      active_layer_ = layer;
      emit layerChanged();
      return true;
    }
  }

  Q_ASSERT_X(false, "Active Layer",
             "Invalid active layer name");
}

bool Scene::setActiveLayer(LayerPtr &layer) {
  if (!layers_.contains(layer)) {
    qInfo() << "Layers" << layers_.size();
    for (auto &layer : layers_) {
      qInfo() << layer.get();
    }
    qInfo() << "Set layer" << layer.get();
    Q_ASSERT_X(false, "Active Layer",
               "Invalid layer ptr when setting active layer");
  }
  active_layer_ = layer;
  emit layerChanged();
  return false;
}

QList<LayerPtr> &Scene::layers() { return layers_; }

void Scene::reorderLayers(QList<LayerPtr> &new_order) {
  layers_.clear();
  layers_.append(new_order);
}

void Scene::removeSelections() {
  // Clear selection pointers in other componenets
  selections().clear();
  emit selectionsChanged();

  // Remove
  for (auto &layer : layers()) {
    layer->children().erase(
         std::remove_if(layer->children().begin(), layer->children().end(),
                        [](ShapePtr &s) { return s->selected(); }),
         layer->children().end());
  }
}

ShapePtr Scene::hitTest(QPointF canvas_coord) {
  for (auto &layer : layers()) {
    for (auto &shape : boost::adaptors::reverse(layer->children())) {
      if (shape->hitTest(canvas_coord, 5 / scale())) {
        return shape;
      }
    }
  }
  return nullptr;
}

QPointF Scene::mousePressedCanvasCoord() const {
  return getCanvasCoord(mouse_pressed_screen_coord_);
}

QPointF Scene::mousePressedScreenCoord() const {
  return mouse_pressed_screen_coord_;
}

void Scene::setMousePressedScreenCoord(QPointF screen_coord) {
  mouse_pressed_screen_coord_ = screen_coord;
}

void Scene::setFont(QFont &font) {
  font_ = font;
}

const QFont &Scene::font() const { return font_; }