#ifndef COMMAND_H
#define COMMAND_H

#include <layer.h>
#include <shape/shape.h>

// TODO add mode change command if we need to do redo in certain modes..

class Document;
// TODO (Move implementation to command.cpp)
namespace Commands {
  class JoinedCmd;

  class BaseCmd {
  public:

    virtual void undo() {
      Q_ASSERT_X(false, "Commands", "This command did not implement undo");
    }

    virtual void redo() {
      Q_ASSERT_X(false, "Commands", "This command did not implement redo");
    }

    shared_ptr<JoinedCmd> operator+(BaseCmd *another_event);
  };

  typedef shared_ptr<BaseCmd> CmdPtr;

  class AddLayer : public BaseCmd {
  public:
    explicit AddLayer(LayerPtr &layer) : layer_(layer) {}

    void undo() override;

    void redo() override;

    LayerPtr layer_;
  };

  class RemoveLayer : public BaseCmd {
  public:
    explicit RemoveLayer(LayerPtr &layer) : layer_(layer) {}

    void undo() override;

    void redo() override;

    LayerPtr layer_;
  };

  class AddShape : public BaseCmd {
  public:
    AddShape(const LayerPtr &layer, const ShapePtr &shape) :
         layer_(layer.get()), shape_(shape) {}

    static shared_ptr<AddShape> shared(const LayerPtr &layer, const ShapePtr &shape) {
      return make_shared<AddShape>(layer, shape);
    }

    void undo() override;

    void redo() override;

    // Shape events don't need to manage layer's lifecycle
    Layer *layer_;
    ShapePtr shape_;
  };

  class RemoveShape : public BaseCmd {
  public:
    explicit RemoveShape(const ShapePtr &shape) :
         shape_(shape) { layer_ = &shape->layer(); }

    RemoveShape(const LayerPtr &layer, const ShapePtr &shape) :
         layer_(layer.get()), shape_(shape) {}

    RemoveShape(Layer *layer, const ShapePtr &shape) :
         layer_(layer), shape_(shape) {}

    static shared_ptr<RemoveShape> shared(const ShapePtr &shape) {
      return make_shared<RemoveShape>(shape);
    }

    static shared_ptr<RemoveShape> shared(const LayerPtr &layer, const ShapePtr &shape) {
      return make_shared<RemoveShape>(layer, shape);
    }

    void undo() override;

    void redo() override;

    // Shape events don't need to manage layer's lifecycle
    Layer *layer_;
    ShapePtr shape_;
  };

  class Select : public BaseCmd {
  public:

    explicit Select(const QList<ShapePtr> &new_selections_);

    static shared_ptr<Select> shared(const QList<ShapePtr> &new_selections) {
      return make_shared<Select>(new_selections);
    }

    void undo() override;

    void redo() override;

    QList<ShapePtr> old_selections_;
    QList<ShapePtr> new_selections_;
  };

  class JoinedCmd : public BaseCmd {
  public:

    JoinedCmd() {}

    // Constructor for joining multiple events
    JoinedCmd(initializer_list<BaseCmd *> undo_events) {
      for (auto &event : undo_events) {
        events << CmdPtr(event);
      }
    }

    // Constructor for joining multiple events (event ptr)
    JoinedCmd(initializer_list<CmdPtr> undo_events) {
      for (auto &event : undo_events) {
        events << event;
      }
    }

    void undo() override {
      for (int i = events.size() - 1; i >= 0; i--) {
        events.at(i)->undo();
      }
    }

    void redo() override {
      for (auto &event : events) {
        event->redo();
      }
    }

    static shared_ptr<JoinedCmd> addShapes(const LayerPtr &layer, const QList<ShapePtr> &shapes) {
      auto evt = make_shared<JoinedCmd>();
      for (auto &shape : shapes) {
        evt->events << make_shared<AddShape>(layer, shape);
      }
      return evt;
    }

    static shared_ptr<JoinedCmd> removeShapes(const QList<ShapePtr> &shapes) {
      auto evt = make_shared<JoinedCmd>();
      for (auto &shape : shapes) {
        evt->events << make_shared<RemoveShape>(shape);
      }
      return evt;
    }

    static shared_ptr<JoinedCmd> removeSelections();

    QList<CmdPtr> events;
  };

  typedef shared_ptr<JoinedCmd> JoinedPtr;

// Event when object's property is changed, and the property can be "passed by value"
  template<typename T, typename PropType, PropType (T::*PropGetter)() const, void (T::*PropSetter)(
       PropType)>
  class Set : public BaseCmd {
  public:

    explicit Set(T *target, PropType new_value) : target_(target), new_value_(new_value) {
      old_value_ = (target_->*PropGetter)();
    }


    static shared_ptr<Set> shared(T *target, PropType new_value) {
      return make_shared<Set>(target, new_value);
    }

    void undo() override {
      qInfo() << "[Command] Undo set";
      (target_->*PropSetter)(old_value_);
    }

    void redo() override {
      qInfo() << "[Command] Do set";
      (target_->*PropSetter)(new_value_);
    };

    T *target_;
    PropType new_value_;
    PropType old_value_;
  };

  // Event when object's property is changed, and the property is usually "passed by reference"
  template<typename T, typename PropType, const PropType &(T::*PropGetter)() const, void (T::*PropSetter)(
       const PropType &)>
  class SetRef : public BaseCmd {
  public:

    explicit SetRef(T *target, PropType new_value) : target_(target), new_value_(new_value) {
      old_value_ = (target_->*PropGetter)();
    }

    static shared_ptr<SetRef> shared(T *target, PropType new_value) {
      return make_shared<SetRef>(target, new_value);
    }

    void undo() override {
      qInfo() << "[Command] Undo setRef";
      (target_->*PropSetter)(old_value_);
    }

    void redo() override {
      qInfo() << "[Command] Do setRef";
      (target_->*PropSetter)(new_value_);
    };

    T *target_;
    PropType new_value_;
    PropType old_value_;
  };

  // Operators overload
  JoinedPtr operator+(const CmdPtr &a, const CmdPtr &b);

  JoinedPtr &operator<<(JoinedPtr &a, const CmdPtr &b);

  JoinedPtr &operator<<(JoinedPtr &a, BaseCmd *b);

  // Abbreviations for undo events
  typedef Commands::SetRef<Shape, QTransform, &Shape::transform, &Shape::setTransform> SetTransform;
  typedef Commands::Set<Shape, qreal, &Shape::rotation, &Shape::setRotation> SetRotation;
}

typedef Commands::CmdPtr CmdPtr;
typedef Commands::JoinedPtr JoinedPtr;
typedef Commands::JoinedCmd JoinedCmd;
typedef Commands::BaseCmd BaseCmd;


#endif