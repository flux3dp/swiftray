#include <QDebug>
#include <boost/range/adaptor/reversed.hpp>
#include <document.h>

Document::Document() noexcept:
     mode_(Mode::Selecting),
     new_layer_id_(1),
     scroll_x_(0),
     scroll_y_(0),
     scale_(1) {
  width_ = 3000;
  height_ = 2000;
  font_ = QFont("Tahoma", 200, QFont::Bold);
  addLayer();
  connect(this, &Document::selectionsChanged, [=]() {
    for (auto &layer : layers_) {
      layer->flushCache();
    }
  });
}

void Document::setSelection(ShapePtr &shape) {
  QList<ShapePtr> list;
  list.push_back(shape);
  setSelections(list);
}

void Document::setSelections(const QList<ShapePtr> &shapes) {
  clearSelections();
  selections().append(shapes);

  for (auto &shape : selections()) {
    shape->setSelected(true);
  }
  emit selectionsChanged();
}

void Document::clearSelections() {
  for (auto &shape : selections()) {
    shape->setSelected(false);
  }

  selections().clear();
  emit selectionsChanged();
}

bool Document::isSelected(ShapePtr &shape) const { return selections_.contains(shape); }

QList<ShapePtr> &Document::selections() { return selections_; }

void Document::clearAll() {
  clearSelections();
  layers().clear();
  active_layer_ = nullptr;
  emit layerChanged();
}

void Document::dumpStack(QList<LayerPtr> &stack) {
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

void Document::undo() {
  if (undo2.isEmpty()) return;
  qInfo() << "Stack" << undo2.size();
  EventPtr evt = undo2.last();
  evt->undo();
  undo2.pop_back();
  redo2 << evt;

  QString active_layer_name = activeLayer()->name();

  // TODO (Fix mode change event and selection)

  setActiveLayer(active_layer_name);
  emit layerChanged();
}

void Document::redo() {
  if (redo2.isEmpty()) return;
  qInfo() << "Stack" << redo2.size();
  EventPtr evt = redo2.last();
  evt->redo();
  redo2.pop_back();
  undo2 << evt;

  QString active_layer_name = activeLayer()->name();

  setActiveLayer(active_layer_name);
  emit layerChanged();
}

void Document::addUndoEvent(BaseUndoEvent *e) {
  // Use shared_ptr to manage lifecycle
  redo2.clear();
  undo2.push_back(shared_ptr<BaseUndoEvent>(e));
};

void Document::addLayer() {
  qDebug() << "Add layer";
  layers() << make_shared<Layer>(new_layer_id_++);
  active_layer_ = layers().last();
  emit layerChanged();
}

void Document::addLayer(LayerPtr &layer) {
  layers() << layer->clone();
  active_layer_ = layers().last();
}

void Document::removeLayer(LayerPtr &layer) {
  if (!layers().removeOne(layer)) {
    qInfo() << "Failed to remove layer";
  }
}

void Document::emitAllChanges() {
  emit selectionsChanged();
  emit layerChanged();
  emit modeChanged();
}

Document::Mode Document::mode() const { return mode_; }

void Document::setMode(Mode mode) {
  mode_ = mode;
  emit modeChanged();
}

QPointF Document::getCanvasCoord(QPointF window_coord) const {
  return (window_coord - scroll()) / scale();
}

QPointF Document::scroll() const { return QPointF(scroll_x_, scroll_y_); }

qreal Document::scrollX() const { return scroll_x_; }

qreal Document::scrollY() const { return scroll_y_; }

qreal Document::scale() const { return scale_; }

qreal Document::width() const { return width_; }

qreal Document::height() const { return height_; }

void Document::setWidth(qreal width) { width_ = width; }

void Document::setHeight(qreal height) { height_ = height; }

void Document::setScroll(QPointF scroll) {
  scroll_x_ = scroll.x();
  scroll_y_ = scroll.y();
}

QRectF Document::screenRect(QSize screen_size) const {
  return QRectF(getCanvasCoord(QPoint(0, 0)), getCanvasCoord(QPoint(screen_size.width(), screen_size.height())));
}

void Document::setScrollX(qreal scroll_x) { scroll_x_ = scroll_x; }

void Document::setScrollY(qreal scroll_y) { scroll_y_ = scroll_y; }

void Document::setScale(qreal scale) { scale_ = scale; }

LayerPtr &Document::activeLayer() {
  Q_ASSERT_X(layers_.size() != 0, "Active Layer",
             "Access to active layer when there is no layer");
  Q_ASSERT_X(active_layer_ != nullptr, "Active Layer",
             "Access to active layer is cleaned");
  return active_layer_;
}

bool Document::setActiveLayer(QString name) {
  for (auto &layer : layers()) {
    if (layer->name() == name) {
      active_layer_ = layer;
      emit layerChanged();
      return true;
    }
  }

  active_layer_ = layers().last();
  return false;
}

bool Document::setActiveLayer(LayerPtr &layer) {
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

QList<LayerPtr> &Document::layers() { return layers_; }

void Document::reorderLayers(QList<LayerPtr> &new_order) {
  layers_.clear();
  layers_.append(new_order);
}

void Document::removeSelections() {
  // Clear selection pointers in other components
  selections().clear();

  // Remove from layer
  for (auto &layer : layers()) {
    layer->removeSelected();
  }

  emit selectionsChanged();
}

ShapePtr Document::hitTest(QPointF canvas_coord) {
  for (auto &layer : layers()) {
    for (auto &shape : boost::adaptors::reverse(layer->children())) {
      if (shape->hitTest(canvas_coord, 5 / scale())) {
        return shape;
      }
    }
  }
  return nullptr;
}

QPointF Document::mousePressedCanvasCoord() const {
  return getCanvasCoord(mouse_pressed_screen_coord_);
}

QPointF Document::mousePressedScreenCoord() const {
  return mouse_pressed_screen_coord_;
}

void Document::setMousePressedScreenCoord(QPointF screen_coord) {
  mouse_pressed_screen_coord_ = screen_coord;
}

void Document::setFont(QFont &font) {
  font_ = font;
}

bool Document::isVolatile() {
  // TODO (Add scrolling and zooming)
  return mode_ == Mode::Moving || mode_ == Mode::Rotating || mode_ == Mode::Transforming;
}

const QFont &Document::font() const { return font_; }