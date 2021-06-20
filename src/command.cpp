#include <QDebug>
#include <command.h>
#include <document.h>

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

void AddLayer::undo(Document *doc) {
  qDebug() << "[Command] Undo add layer";
  doc->removeLayer(layer_);
}

void AddLayer::redo(Document *doc) {
  qDebug() << "[Command] Do add layer";
  doc->addLayer(layer_);
}

void RemoveLayer::undo(Document *doc) {
  doc->addLayer(layer_);
}

void RemoveLayer::redo(Document *doc) {
  doc->removeLayer(layer_);
}

void AddShape::undo(Document *doc) {
  qDebug() << "[Command] Undo add shape" << shape_.get();
  layer_->removeShape(shape_);
}

void AddShape::redo(Document *doc) {
  qDebug() << "[Command] Do add shape" << shape_.get();
  layer_->addShape(shape_);
}


void RemoveShape::undo(Document *doc) {
  qDebug() << "[Command] Undo remove shape";
  layer_->addShape(shape_);
}

void RemoveShape::redo(Document *doc) {
  qDebug() << "[Command] Do remove shape";
  layer_->removeShape(shape_);
}

Select::Select(Document *doc, const QList<ShapePtr> &new_selections) {
  old_selections_.clear();
  old_selections_.append(doc->selections());
  new_selections_.clear();
  new_selections_.append(new_selections);
}

void Select::undo(Document *doc) {
  qDebug() << "[Command] Undo select";
  doc->setSelections(old_selections_);
}

void Select::redo(Document *doc) {
  if (!new_selections_.empty()) {
    qDebug() << "[Command] Do select" << new_selections_.first().get();
  } else {
    qDebug() << "[Command] Do select none";
  }
  doc->setSelections(new_selections_);
}

shared_ptr<JoinedCmd> JoinedCmd::removeSelections(Document *doc) {
  QList<ShapePtr> selections;
  selections.append(doc->selections());
  return Select::shared(doc, {}) +
         JoinedCmd::removeShapes(selections);
}