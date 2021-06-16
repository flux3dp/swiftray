#ifndef UNDO_EVENT_H
#define UNDO_EVENT_H

#include <layer.h>
#include <shape/shape.h>

class Document;

class BaseUndoEvent {
public:
  enum class Type {
    AddElement,
    RemoveElement,
    AttributeChange,
    ContentChange,
    ChildrenOrderChange,
    None
  };

  virtual void undo() {}

  virtual void redo() {}

  virtual Type type() { return Type::None; }
};

typedef shared_ptr<BaseUndoEvent> EventPtr;

class AddLayerEvent : public BaseUndoEvent {
public:
  AddLayerEvent(Document &document, LayerPtr layer) :
       document_(document), layer_(layer) {}

  virtual void undo();

  virtual void redo();

  LayerPtr layer_;
  Document &document_;
};

class RemoveLayerEvent : public BaseUndoEvent {
public:
  RemoveLayerEvent(Document &document, LayerPtr layer) :
       document_(document), layer_(layer) {}

  virtual void undo();

  virtual void redo();

  LayerPtr layer_;
  Document &document_;
};

class AddShapeEvent : public BaseUndoEvent {
public:
  AddShapeEvent(ShapePtr shape) :
       shape_(shape) { layer_ = shape->parent(); }

  AddShapeEvent(LayerPtr layer, ShapePtr shape) :
       layer_(layer.get()), shape_(shape) {}

  virtual void undo() {
    qInfo() << "Undoing add shape" << shape_.get() << "with layer" << layer_->name();
    layer_->removeShape(shape_);
  }

  virtual void redo() {
    qInfo() << "Redoing add shape";
    layer_->addShape(shape_);
  }

  // Shape events don't need to manage layer's lifecycle
  Layer *layer_;
  ShapePtr shape_;
};

class RemoveShapeEvent : public BaseUndoEvent {
public:
  RemoveShapeEvent(ShapePtr shape) :
       shape_(shape) {}

  RemoveShapeEvent(LayerPtr layer, ShapePtr shape) :
       layer_(layer.get()), shape_(shape) {}

  virtual void undo() {
    shape_->parent()->addShape(shape_);
  }

  virtual void redo() {
    shape_->parent()->removeShape(shape_);
  }

  // Shape events don't need to manage layer's lifecycle
  Layer *layer_;
  ShapePtr shape_;
};

class JoinedEvent : public BaseUndoEvent {
public:
  virtual void undo() {
    for (auto &event : events) {
      event->undo();
    }
  }

  virtual void redo() {
    for (auto &event : events) {
      event->redo();
    }
  }

  QList<EventPtr> events;
};


template<typename T, typename PropType, const PropType &(T::*PropGetter)() const, void (T::*PropSetter)(
     const PropType &)>
class PropChangeEvent : public BaseUndoEvent {
public:

  explicit PropChangeEvent(shared_ptr<T> target, PropType value) {
    target_ = target;
    value_ = value;
    qInfo() << "New PCE" << target.get();
  }

  void undo() override {
    qInfo() << "Undoing prop change";
    redo_value_ = (target_.get()->*PropGetter)();
    (target_.get()->*PropSetter)(value_);
  }

  void redo() override {
    qInfo() << "Redoing prop change";
    (target_.get()->*PropSetter)(redo_value_);
  };

  shared_ptr<T> target_;
  PropType value_;
  PropType redo_value_;
};

#endif