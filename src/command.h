#pragma once

#include <layer.h>
#include <shape/shape.h>
#include <shape/text-shape.h>

// TODO (Create batch command for add / remove shapes)

class Document;

/**
    \namespace Commands
    \brief A command represent a scene operation with undo/redo implementation.
*/
namespace Commands {

  /**
      \class BaseCmd
      \brief BaseCmd represents the template for all commands
  */
  class BaseCmd {
  public:

    // BaseCmd 
    BaseCmd() = default;

    virtual void undo(Document *doc) {
      Q_ASSERT_X(false, "Commands", "This command did not implement undo");
    }

    virtual void redo(Document *doc) {
      Q_ASSERT_X(false, "Commands", "This command did not implement redo");
    }
  };

  typedef std::shared_ptr<BaseCmd> CmdPtr;


  /**
      \class AddLayerCmd
      \brief AddLayerCmd add layers and manages lifecycles of layers.
  */
  class AddLayerCmd : public BaseCmd {
  public:
    AddLayerCmd(const LayerPtr &layer) : layer_(layer) {}

    void undo(Document *doc) override;

    void redo(Document *doc) override;

    LayerPtr layer_;
  };

  /**
      \class RemoveLayerCmd
      \brief RemoveLayerCmd remove layers and manages lifecycles of layers.
  */
  class RemoveLayerCmd : public BaseCmd {
  public:
    RemoveLayerCmd(const LayerPtr &layer) : layer_(layer) {}

    void undo(Document *doc) override;

    void redo(Document *doc) override;

    LayerPtr layer_;
  };

  /**
      \class AddShapeCmd
      \brief AddShapeCmd adds shapes and manages lifecycles of shapes.
  */
  class AddShapeCmd : public BaseCmd {
  public:
    AddShapeCmd(Layer *layer, const ShapePtr &shape) :
         layer_(layer), shape_(shape) {}

    void undo(Document *doc) override;

    void redo(Document *doc) override;

    Layer *layer_;
    ShapePtr shape_;
  };

  /**
      \class RemoveShapeCmd
      \brief RemoveShapeCmd removes shapes and manages lifecycles of shapes
  */
  class RemoveShapeCmd : public BaseCmd {
  public:
    explicit RemoveShapeCmd(const ShapePtr &shape) :
         shape_(shape) { layer_ = shape->layer(); }

    RemoveShapeCmd(Layer *layer, const ShapePtr &shape) :
         layer_(layer), shape_(shape) {}

    void undo(Document *doc) override;

    void redo(Document *doc) override;

    Layer *layer_;
    ShapePtr shape_;
  };

  /**
      \class SelectCmd
      \brief SelectCmd change selections.
  */
  class SelectCmd : public BaseCmd {
  public:

    explicit SelectCmd(Document *doc, const QList<ShapePtr> &new_selections_);

    void undo(Document *doc) override;

    void redo(Document *doc) override;

    QList<ShapePtr> old_selections_;
    QList<ShapePtr> new_selections_;
    QMutex old_select_mutex_;
    QMutex new_select_mutex_;
  };

  /**
      \class JoinedCmd
      \brief JoinedCmd represents <b>a group of commands</b> that can be considered as a single step in undo/redo
  */
  class JoinedCmd : public BaseCmd {
  public:

    JoinedCmd() = default;

    JoinedCmd(std::initializer_list<BaseCmd *> undo_events);

    JoinedCmd(std::initializer_list<CmdPtr> undo_events);

    void undo(Document *doc) override;

    void redo(Document *doc) override;

    QList<CmdPtr> events;
  };

  typedef std::shared_ptr<JoinedCmd> JoinedPtr;

  /**
      \class SetCmd
      \brief SetCmd changes a property of an object, and the property can be <b>passed by value</b>
  */
  template<typename T, typename PropType, PropType (T::*PropGetter)() const, void (T::*PropSetter)(
       PropType)>
  class SetCmd : public BaseCmd {
  public:

    explicit SetCmd(T *target, PropType new_value) : target_(target), new_value_(new_value) {
      old_value_ = (target_->*PropGetter)();
    }

    void undo(Document *doc) override {
      qInfo() << "[Command] Undo set";
      (target_->*PropSetter)(old_value_);
    }

    void redo(Document *doc) override {
      qInfo() << "[Command] Do set";
      (target_->*PropSetter)(new_value_);
    };

    T *target_;
    PropType new_value_;
    PropType old_value_;
  };

  /**
      \class SetRefCmd
      \brief SetRef changes a property of an object, and the property is usually <b>passed by reference</b>
  */
  template<typename T, typename PropType, const PropType &(T::*PropGetter)() const, void (T::*PropSetter)(
       const PropType &)>
  class SetRefCmd : public BaseCmd {
  public:

    explicit SetRefCmd(T *target, PropType new_value) : target_(target), new_value_(new_value) {
      old_value_ = (target_->*PropGetter)();
    }

    void undo(Document *doc) override {
      qInfo() << "[Command] Undo setRef";
      (target_->*PropSetter)(old_value_);
    }

    void redo(Document *doc) override {
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

  // Abbreviations for generating commands
  template<typename T, typename PropType, PropType (T::*PropGetter)() const, void (T::*PropSetter)(
       PropType)>
  CmdPtr Set(T *target, PropType new_value) {
    return std::make_shared<SetCmd<T, PropType, PropGetter, PropSetter>>(target, new_value);
  }

  template<typename T, typename PropType, const PropType &(T::*PropGetter)() const, void (T::*PropSetter)(
       const PropType &)>
  CmdPtr SetRef(T *target, PropType new_value) {
    return std::make_shared<SetRefCmd<T, PropType, PropGetter, PropSetter>>
         (target, new_value);
  }

  // Abbreviations for specific set
  constexpr CmdPtr
  (*SetTransform)(Shape *, QTransform) =
  &SetRef<Shape, QTransform, &Shape::transform, &Shape::setTransform>;

  constexpr CmdPtr (*SetParent)(Shape *, Shape *) =
  &Set<Shape, Shape *, &Shape::parent, &Shape::setParent>;

  constexpr CmdPtr (*SetLayer)(Shape *, Layer *) =
  &Set<Shape, Layer *, &Shape::layer, &Shape::setLayer>;

  constexpr CmdPtr (*SetFont)(TextShape *, QFont) =
  &SetRef<TextShape, QFont, &TextShape::font, &TextShape::setFont>;

  constexpr CmdPtr (*SetLineHeight)(TextShape *, float) =
  &Set<TextShape, float, &TextShape::lineHeight, &TextShape::setLineHeight>;

  constexpr CmdPtr (*SetRotation)(Shape *, qreal) =
  &Set<Shape, qreal, &TextShape::rotation, &TextShape::setRotation>;


  // Construct the command object within namespace functions for better readability in other places
  CmdPtr AddShape(Layer *layer, const ShapePtr &shape);

  CmdPtr RemoveShape(const ShapePtr &shape);

  CmdPtr RemoveShape(Layer *layer, const ShapePtr &shape);

  CmdPtr Select(Document *doc, const QList<ShapePtr> &new_selections);

  template<typename T, typename PropType, PropType (T::*PropGetter)() const, void (T::*PropSetter)(
       PropType)>
  CmdPtr Set(T *target, PropType new_value);

  template<typename T, typename PropType, const PropType &(T::*PropGetter)() const, void (T::*PropSetter)(
       const PropType &)>
  CmdPtr SetRef(T *target, PropType new_value);

  CmdPtr AddShapes(Layer *layer, const QList<ShapePtr> &shapes);

  CmdPtr RemoveShapes(const QList<ShapePtr> &shapes);

  CmdPtr AddLayer(const LayerPtr &layer);

  CmdPtr RemoveLayer(const LayerPtr &layer);

  CmdPtr RemoveSelections(Document *doc);

  JoinedPtr Joined();
}

typedef Commands::CmdPtr CmdPtr;