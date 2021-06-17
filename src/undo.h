#ifndef UNDO_EVENT_H
#define UNDO_EVENT_H

#include <layer.h>
#include <shape/shape.h>

class Document;

class JoinedEvent;

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

  shared_ptr<JoinedEvent> operator+(BaseUndoEvent *another_event);
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

  static shared_ptr<AddShapeEvent> shared(ShapePtr shape) {
    return make_shared<AddShapeEvent>(shape);
  }

  static shared_ptr<AddShapeEvent> shared(LayerPtr layer, ShapePtr shape) {
    return make_shared<AddShapeEvent>(layer, shape);
  }

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

class SelectionEvent : public BaseUndoEvent {
public:
  SelectionEvent();

  SelectionEvent(const QList<ShapePtr> &origin_selections) {
    origin_selections_.clear();
    origin_selections_.append(origin_selections);
  }

  static shared_ptr<SelectionEvent> shared(const QList<ShapePtr> &origin_selections) {
    return make_shared<SelectionEvent>(origin_selections);
  }

  static shared_ptr<SelectionEvent> changeFromCurrent() {
    return make_shared<SelectionEvent>();
  }

  void undo() override;

  void redo() override;

  QList<ShapePtr> origin_selections_;
  QList<ShapePtr> redo_selections_;
};

class JoinedEvent : public BaseUndoEvent {
public:

  JoinedEvent() {}

  // Constructor for joining multiple events
  JoinedEvent(initializer_list<BaseUndoEvent *> undo_events) {
    for (auto &event : undo_events) {
      events << EventPtr(event);
    }
  }

  // Constructor for joining multiple events (event ptr)
  JoinedEvent(initializer_list<EventPtr> undo_events) {
    for (auto &event : undo_events) {
      events << event;
    }
  }

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

  static shared_ptr<JoinedEvent> addShapes(const QList<ShapePtr> &shapes) {
    auto evt = make_shared<JoinedEvent>();
    for (auto &shape : shapes) {
      evt->events << make_shared<AddShapeEvent>(shape);
    }
    return evt;
  }

  static shared_ptr<JoinedEvent> removeShapes(const QList<ShapePtr> &shapes) {
    auto evt = make_shared<JoinedEvent>();
    for (auto &shape : shapes) {
      evt->events << make_shared<RemoveShapeEvent>(shape);
    }
    return evt;
  }

  QList<EventPtr> events;
};

typedef shared_ptr<JoinedEvent> JoinedEventPtr;

// TODO(Add prop change event for non referenced type, probably change PropType to PropType&)
template<typename T, typename PropType, PropType (T::*PropGetter)() const, void (T::*PropSetter)(
     PropType)>
class PropChangeEvent : public BaseUndoEvent {
public:

  explicit PropChangeEvent(T *target, PropType value) : target_(target), value_(value) {}

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

template<typename T, typename PropType, const PropType &(T::*PropGetter)() const, void (T::*PropSetter)(
     const PropType &)>
class PropObjChangeEvent : public BaseUndoEvent {
public:

  explicit PropObjChangeEvent(T *target, PropType value) : target_(target), value_(value) {}

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

// Operators overload
JoinedEventPtr operator+(const EventPtr &a, const EventPtr &b);

JoinedEventPtr &operator<<(JoinedEventPtr &a, const EventPtr &b);

JoinedEventPtr &operator<<(JoinedEventPtr &a, BaseUndoEvent *b);

// Abbreviations for undo events
typedef PropObjChangeEvent<Shape, QTransform, &Shape::transform, &Shape::setTransform> TransformChangeEvent;
typedef PropChangeEvent<Shape, qreal, &Shape::rotation, &Shape::setRotation> RotationChangeEvent;

#endif