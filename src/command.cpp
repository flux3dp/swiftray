#include <QDebug>
#include <command.h>
#include <document.h>

using namespace Commands;

// Operators
JoinedPtr Commands::operator+(const CmdPtr &a, const CmdPtr &b) {
  return JoinedPtr(new JoinedCmd({a, b}));
}

JoinedPtr &Commands::operator<<(JoinedPtr &a, const CmdPtr &b) {
  a->events << b;
  return a;
}

JoinedPtr &Commands::operator<<(JoinedPtr &a, BaseCmd *b) {
  a->events << CmdPtr(b);
  return a;
}

void AddLayerCmd::undo(Document *doc) {
  qDebug() << "[Command] Undo add layer";
  doc->removeLayer(layer_);
}

void AddLayerCmd::redo(Document *doc) {
  qDebug() << "[Command] Do add layer";
  doc->addLayer(layer_);
}

void RemoveLayerCmd::undo(Document *doc) {
  doc->addLayer(layer_);
}

void RemoveLayerCmd::redo(Document *doc) {
  doc->removeLayer(layer_);
}

void AddShapeCmd::undo(Document *doc) {
  qDebug() << "[Command] Undo add shape" << shape_.get();
  layer_->removeShape(shape_);
}

void AddShapeCmd::redo(Document *doc) {
  qDebug() << "[Command] Do add shape" << shape_.get();
  layer_->addShape(shape_);
}


void RemoveShapeCmd::undo(Document *doc) {
  qDebug() << "[Command] Undo remove shape";
  layer_->addShape(shape_);
}

void RemoveShapeCmd::redo(Document *doc) {
  qDebug() << "[Command] Do remove shape";
  layer_->removeShape(shape_);
}

SelectCmd::SelectCmd(Document *doc, const QList<ShapePtr> &new_selections) {
  old_selections_.clear();
  old_selections_.append(doc->selections());
  new_selections_.clear();
  new_selections_.append(new_selections);
}

void SelectCmd::undo(Document *doc) {
  qDebug() << "[Command] Undo select";
  doc->setSelections(old_selections_);
}

void SelectCmd::redo(Document *doc) {
  if (!new_selections_.empty()) {
    qDebug() << "[Command] Do select" << new_selections_.first().get();
  } else {
    qDebug() << "[Command] Do select none";
  }
  doc->setSelections(new_selections_);
}

CmdPtr Commands::RemoveSelections(Document *doc) {
  QList<ShapePtr> selections;
  selections.append(doc->selections());
  return Commands::Select(doc, {}) +
         Commands::RemoveShapes(selections);
}


CmdPtr Commands::SetTransform(Shape *shape, const QTransform &new_value) {
  return make_shared<SetTransformCmd>(shape, new_value);
}

CmdPtr Commands::SetLayer(Shape *shape, Layer *layer) {
  return make_shared<SetLayerCmd>(shape, layer);
}

CmdPtr Commands::SetParent(Shape *shape, Shape *parent) {
  return make_shared<SetParentCmd>(shape, parent);
}

CmdPtr Commands::SetRotation(Shape *shape, qreal rotation) {
  return make_shared<SetRotationCmd>(shape, rotation);
}

CmdPtr Commands::AddShape(Layer *layer, const ShapePtr &shape) {
  return make_shared<AddShapeCmd>(layer, shape);
}

CmdPtr Commands::RemoveShape(const ShapePtr &shape) {
  return make_shared<RemoveShapeCmd>(shape);
}

CmdPtr Commands::RemoveShape(Layer *layer, const ShapePtr &shape) {
  return make_shared<RemoveShapeCmd>(layer, shape);
}

CmdPtr Commands::Select(Document *doc, const QList<ShapePtr> &new_selections) {
  return make_shared<SelectCmd>(doc, new_selections);
}

CmdPtr Commands::AddShapes(Layer *layer, const QList<ShapePtr> &shapes) {
  auto evt = make_shared<JoinedCmd>();
  for (auto &shape : shapes) {
    evt->events << make_shared<AddShapeCmd>(layer, shape);
  }
  return evt;
}

CmdPtr Commands::RemoveShapes(const QList<ShapePtr> &shapes) {
  auto evt = make_shared<JoinedCmd>();
  for (auto &shape : shapes) {
    evt->events << Commands::RemoveShape(shape);
  }
  return evt;
}

CmdPtr Commands::AddLayer(const LayerPtr &layer) {
  return make_shared<AddLayerCmd>(layer);
}

CmdPtr Commands::RemoveLayer(const LayerPtr &layer) {
  return make_shared<RemoveLayerCmd>(layer);
}
