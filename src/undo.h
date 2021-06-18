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
  explicit AddLayerEvent(LayerPtr &layer) : layer_(layer) {}

  void undo() override;

  void redo() override;

  LayerPtr layer_;
};

class RemoveLayerEvent : public BaseUndoEvent {
public:
  explicit RemoveLayerEvent(LayerPtr &layer) : layer_(layer) {}

  void undo() override;

  void redo() override;

  LayerPtr layer_;
};

class AddShapeEvent : public BaseUndoEvent {
public:
  explicit AddShapeEvent(const ShapePtr &shape) :
       shape_(shape) { layer_ = &shape->layer(); }

  AddShapeEvent(const LayerPtr &layer, const ShapePtr &shape) :
       layer_(layer.get()), shape_(shape) {}

  static shared_ptr<AddShapeEvent> shared(const ShapePtr &shape) {
    return make_shared<AddShapeEvent>(shape);
  }

  static shared_ptr<AddShapeEvent> shared(const LayerPtr &layer, const ShapePtr &shape) {
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
  explicit RemoveShapeEvent(const ShapePtr &shape) :
       shape_(shape) { layer_ = &shape->layer(); }

  RemoveShapeEvent(const LayerPtr &layer, const ShapePtr &shape) :
       layer_(layer.get()), shape_(shape) {}

  static shared_ptr<RemoveShapeEvent> shared(const ShapePtr &shape) {
    return make_shared<RemoveShapeEvent>(shape);
  }

  static shared_ptr<RemoveShapeEvent> shared(const LayerPtr &layer, const ShapePtr &shape) {
    return make_shared<RemoveShapeEvent>(layer, shape);
  }

  void undo() override;

  void redo() override;

  // Shape events don't need to manage layer's lifecycle
  Layer *layer_;
  ShapePtr shape_;
};

class SelectionEvent : public BaseUndoEvent {
public:
  SelectionEvent();

  explicit SelectionEvent(const QList<ShapePtr> &origin_selections) {
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

  void undo() override {
    for (auto &event : events) {
      event->undo();
    }
  }

  void redo() override {
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

// Event when object's property is changed, and the property can be "passed by value"
template<typename T, typename PropType, PropType (T::*PropGetter)() const, void (T::*PropSetter)(
     PropType)>
class PropEvent : public BaseUndoEvent {
public:

  explicit PropEvent(T *target, PropType value) : target_(target), value_(value) {}

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

// Event when object's property is changed, and the property is usually "passed by reference"
template<typename T, typename PropType, const PropType &(T::*PropGetter)() const, void (T::*PropSetter)(
     const PropType &)>
class PropObjEvent : public BaseUndoEvent {
public:

  explicit PropObjEvent(T *target, PropType value) : target_(target), value_(value) {}

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
typedef PropObjEvent<Shape, QTransform, &Shape::transform, &Shape::setTransform> TransformChangeEvent;
typedef PropEvent<Shape, qreal, &Shape::rotation, &Shape::setRotation> RotationChangeEvent;

#endif