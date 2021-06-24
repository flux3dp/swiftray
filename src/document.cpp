#include <QDebug>
#include <boost/range/adaptor/reversed.hpp>
#include <document.h>
#include <shape/group-shape.h>

Document::Document() noexcept:
     new_layer_id_(1),
     scroll_x_(0),
     scroll_y_(0),
     scale_(1),
     screen_changed_(false),
     frames_count_(0),
     width_(3000),
     height_(2000),
     font_(QFont("Tahoma", 200, QFont::Bold)),
     active_layer_(nullptr),
     canvas_(nullptr) {
  auto layer1 = make_shared<Layer>(this, 1);
  addLayer(layer1);
}

void Document::setSelection(nullptr_t) {
  setSelections({});
}

void Document::setSelection(ShapePtr &shape) {
  setSelections({shape});
}

void Document::setSelections(const QList<ShapePtr> &new_selections) {
  for (auto &shape : selections_) { shape->setSelected(false); }
  selections_ = new_selections;
  for (auto &shape : selections_) { shape->setSelected(true); }
  emit selectionsChanged();
}

QList<ShapePtr> &Document::selections() { return selections_; }

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
  if (undo2_stack_.isEmpty()) {
    qDebug() << "[Document] There is no command to undo!";
    return;
  }
  CmdPtr evt = undo2_stack_.last();
  evt->undo(this);
  undo2_stack_.pop_back();
  redo2_stack_ << evt;

  QString active_layer_name = activeLayer()->name();

  // TODO (Fix mode change event and selection, and add layer)
  setActiveLayer(active_layer_name);
}

void Document::redo() {
  if (redo2_stack_.isEmpty()) return;
  CmdPtr evt = redo2_stack_.last();
  evt->redo(this);
  redo2_stack_.pop_back();
  undo2_stack_ << evt;

  QString active_layer_name = activeLayer()->name();

  setActiveLayer(active_layer_name);
}

void Document::execute(Commands::BaseCmd *cmd) {
  execute(CmdPtr(cmd));
}

void Document::execute(const CmdPtr &cmd) {
  redo2_stack_.clear();
  cmd->redo(this);
  undo2_stack_.push_back(cmd);
}

void Document::execute(initializer_list<CmdPtr> cmds) {
  auto joined = Commands::Joined();
  for (auto cmd: cmds) {
    joined << cmd;
  }
  execute(joined);
}

void Document::addLayer(LayerPtr &layer) {
  layer->setDocument(this);
  layers_ << layer;
  active_layer_ = layers().last().get();
}

void Document::removeLayer(LayerPtr &layer) {
  if (!layers_.removeOne(layer)) {
    qInfo() << "Failed to remove layer";
  }
}

QPointF Document::getCanvasCoord(QPointF window_coord) const {
  return (window_coord - scroll()) / scale();
}

QPointF Document::scroll() const { return QPointF(scroll_x_, scroll_y_); }

qreal Document::scale() const { return scale_; }

qreal Document::width() const { return width_; }

qreal Document::height() const { return height_; }

void Document::setWidth(qreal width) { width_ = width; }

void Document::setHeight(qreal height) { height_ = height; }

void Document::setScroll(QPointF scroll) {
  scroll_x_ = scroll.x();
  scroll_y_ = scroll.y();
  screen_changed_ = true;
}

void Document::setScreenSize(QSize size) {
  screen_size_ = size;
  screen_changed_ = true;
}

void Document::setScale(qreal scale) {
  scale_ = scale;
  screen_changed_ = true;
}

Layer *Document::activeLayer() {
  Q_ASSERT_X(layers_.size() != 0, "Active Layer",
             "Access to active layer when there is no layer");
  Q_ASSERT_X(active_layer_ != nullptr, "Active Layer",
             "Access to active layer is cleaned");
  return active_layer_;
}

bool Document::setActiveLayer(const QString &name) {
  auto layer_ptr = findLayerByName(name);
  if (layer_ptr != nullptr) {
    active_layer_ = layer_ptr->get();
    return true;
  }

  active_layer_ = layers().last().get();
  return false;
}

void Document::setActiveLayer(LayerPtr &target_layer) {
  if (!layers_.contains(target_layer)) {
    qInfo() << "Total layers" << layers_.size();
    for (auto &layer : layers_) {
      qInfo() << layer.get();
    }
    qInfo() << "Cannot set active layer" << target_layer.get();
    Q_ASSERT_X(false, "Active Layer",
               "Invalid layer ptr when setting active layer");
  }
  active_layer_ = target_layer.get();
}

const QList<LayerPtr> &Document::layers() const { return layers_; }

void Document::setLayersOrder(const QList<LayerPtr> &new_order) {
  // TODO (Add undo event)
  layers_ = new_order;
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

void Document::groupSelections() {
  if (selections().empty()) return;

  ShapePtr group_ptr = make_shared<GroupShape>(selections());
  auto cmd = Commands::Joined();
  for (auto &shape: selections()) {
    cmd << Commands::SetParent(shape.get(), group_ptr.get());
    cmd << Commands::SetLayer(shape.get(), nullptr);
  }
  execute(
       cmd,
       Commands::AddShape(activeLayer(), group_ptr),
       Commands::RemoveSelections(this),
       Commands::Select(this, {group_ptr})
  );
}

void Document::ungroupSelections() {
  if (selections().empty()) return;
  auto cmd = Commands::Joined();
  QList<ShapePtr> shapes_added_back;
  for (auto &shape: selections()) {
    if (shape->type() != Shape::Type::Group) return;
    auto *group = (GroupShape *) shape.get();
    for (auto &shape : group->children()) {
      cmd << Commands::SetTransform(shape.get(), shape->transform() * group->transform());
      cmd << Commands::SetRotation(shape.get(), shape->rotation() + group->rotation());
      cmd << Commands::SetParent(shape.get(), nullptr);
      cmd << Commands::AddShape(activeLayer(), shape);
      shapes_added_back << shape;
    }
    cmd << Commands::RemoveShape(shape->layer(), shape);
  }
  cmd << Commands::Select(this, shapes_added_back);
  execute(cmd);
}

void Document::paint(QPainter *painter) {
  frames_count_++;

  int object_count = 0;
  for (const LayerPtr &layer : layers()) {
    if (screen_changed_) layer->flushCache();
    object_count += layer->paint(painter);
  }

  screen_changed_ = false;
}

int Document::framesCount() const {
  return frames_count_;
}

QRectF Document::screenRect() const {
  return QRectF(getCanvasCoord(QPoint(0, 0)),
                getCanvasCoord(QPoint(screen_size_.width(), screen_size_.height())));
}

const LayerPtr *Document::findLayerByName(const QString &layer_name) {
  for (auto &layer : layers_) {
    if (layer->name() == layer_name) return &layer;
  }
  return nullptr;
}

const Canvas *Document::canvas() const {
  return canvas_;
}

void Document::setCanvas(Canvas *canvas) {
  canvas_ = canvas;
}