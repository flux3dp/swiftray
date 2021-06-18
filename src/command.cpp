#include <canvas/canvas.h>

using namespace Commands;

// Operators
// TODO (Use arg list instead of +)
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

void AddLayer::undo() {
  qDebug() << "[Command] Undo add layer";
  Canvas::document().removeLayer(layer_);
}

void AddLayer::redo() {
  qDebug() << "[Command] Do add layer";
  Canvas::document().addLayer(layer_);
}

void RemoveLayer::undo() {
  Canvas::document().addLayer(layer_);
}

void RemoveLayer::redo() {
  Canvas::document().removeLayer(layer_);
}

void AddShape::undo() {
  qDebug() << "[Command] Undo remove shape";
  layer_->removeShape(shape_);
}

void AddShape::redo() {
  qDebug() << "[Command] Do add shape";
  layer_->addShape(shape_);
}


void RemoveShape::undo() {
  qDebug() << "[Command] Undo remove shape";
  layer_->addShape(shape_);
}

void RemoveShape::redo() {
  qDebug() << "[Command] Do remove shape";
  layer_->removeShape(shape_);
}

Select::Select(const QList<ShapePtr> &new_selections) {
  old_selections_.clear();
  old_selections_.append(Canvas::document().selections());
  new_selections_.clear();
  new_selections_.append(new_selections);
}

void Select::undo() {
  qDebug() << "[Command] Undo select";
  Canvas::document().setSelections(old_selections_);
}

void Select::redo() {
  qDebug() << "[Command] Do select";
  Canvas::document().setSelections(new_selections_);
}

shared_ptr<JoinedCmd> JoinedCmd::removeSelections() {
  QList<ShapePtr> selections;
  selections.append(Canvas::document().selections());
  return Select::shared({}) +
         JoinedCmd::removeShapes(selections);
}