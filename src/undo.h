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
  AddLayerEvent(LayerPtr &layer) : layer_(layer) {}

  void undo() override;

  void redo() override;

  LayerPtr layer_;
};

class RemoveLayerEvent : public BaseUndoEvent {
public:
  RemoveLayerEvent(LayerPtr layer) : layer_(layer) {}

  void undo() override;

  void redo() override;

  LayerPtr layer_;
};

class AddShapeEvent : public BaseUndoEvent {
public:
  AddShapeEvent(ShapePtr shape) :
       shape_(shape) { layer_ = &shape->layer(); }

  AddShapeEvent(LayerPtr layer, ShapePtr shape) :
       layer_(layer.get()), shape_(shape) {}

  void undo() override;

  void redo() override;

  // Shape events don't need to manage layer's lifecycle
  Layer *layer_;
  ShapePtr shape_;
};

class RemoveShapeEvent : public BaseUndoEvent {
public:
  RemoveShapeEvent(ShapePtr shape) :
       shape_(shape) { layer_ = &shape->layer(); }

  RemoveShapeEvent(LayerPtr layer, ShapePtr shape) :
       layer_(layer.get()), shape_(shape) {}

  void undo() override;

  void redo() override;

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

  static JoinedEvent *addShapes(const QList<ShapePtr> &shapes) {
    JoinedEvent *evt = new JoinedEvent();
    for (auto &shape : shapes) {
      evt->events << make_shared<AddShapeEvent>(shape);
    }
    return evt;
  }

  static JoinedEvent *removeShapes(const QList<ShapePtr> &shapes) {
    JoinedEvent *evt = new JoinedEvent();
    for (auto &shape : shapes) {
      evt->events << make_shared<RemoveShapeEvent>(shape);
    }
    return evt;
  }

  QList<EventPtr> events;
};


template<typename T, typename PropType, const PropType &(T::*PropGetter)() const, void (T::*PropSetter)(
     const PropType &)>
class PropChangeEvent : public BaseUndoEvent {
public:

  explicit PropChangeEvent(T *target, PropType value) {
    target_ = target;
    value_ = value;
    qInfo() << "New PropChangeEvent (shape" << target << ")";
  }

  void undo() override {
    qInfo() << "Undoing prop change";
    redo_value_ = (target_->*PropGetter)();
    (target_->*PropSetter)(value_);
  }

  void redo() override {
    qInfo() << "Redoing prop change";
    (target_->*PropSetter)(redo_value_);
  };

  T *target_;
  PropType value_;
  PropType redo_value_;
};

#endif