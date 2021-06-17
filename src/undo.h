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

class SelectionEvent : public BaseUndoEvent {
public:
  SelectionEvent();

  SelectionEvent(QList<ShapePtr> &origin_selections) {
    origin_selections_.clear();
    origin_selections_.append(origin_selections);
  }

  void undo() override;

  void redo() override;

  QList<ShapePtr> origin_selections_;
  QList<ShapePtr> redo_selections_;
};

class JoinedEvent : public BaseUndoEvent {
public:

  JoinedEvent() {}

  // Constructor for joining two events
  JoinedEvent(BaseUndoEvent *e1, BaseUndoEvent *e2) {
    events << EventPtr(e1);
    events << EventPtr(e2);
  }

  // Constructor for joining three events
  JoinedEvent(BaseUndoEvent *e1, BaseUndoEvent *e2, BaseUndoEvent *e3) : JoinedEvent(e1, e2) {
    events << EventPtr(e3);
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

#endif