#ifndef CACHE_STACK_H
#define CACHE_STACK_H

#include <QDebug>
#include <QList>
#include <shape/path-shape.h>

class CacheGroup {
public:
  enum class Type {
    SelectedPaths,
    NonSelectedPaths,
    FilledPaths,
    Bitmap,
    Group
  };

  explicit CacheGroup(Type type) : type_(type), count_(0) {
  }

  void merge(QRectF screen_rect) {
    if (type_ != Type::SelectedPaths && type_ != Type::NonSelectedPaths) return;
    for (auto &shape : shapes_) {
      PathShape *p = (PathShape *) shape;
      QTransform transform = p->selected() ? p->transform() * p->tempTransform() : p->transform();
      QPainterPath transformed_path = transform.map(p->path());
      if (transformed_path.intersects(screen_rect)) {
        joined_path_.addPath(transformed_path);
        count_++;
      }
    }
  }

  void paint(QPainter *painter) {
    if (type_ == Type::SelectedPaths || type_ == Type::NonSelectedPaths) {
      // qInfo() << "CacheStack" << this << "joined paint" << count_;
      painter->drawPath(joined_path_);
    } else {
      // qInfo() << "CacheStack" << this << "sep paint";
      for (auto &shape : shapes_) {
        shape->paint(painter);
      }
    }
  }

  // Use weak pointer (no lifespan issue here)
  QList<Shape *> shapes_;
  Type type_;
  int count_;
  QPainterPath joined_path_;
};

typedef CacheGroup::Type CacheType;

class CacheStack {
public:
  void begin(QRectF screen_rect) {
    screen_rect_ = screen_rect;
    groups_.clear();
  }

  CacheType getGroupType(Shape *shape) {
    if (shape->type() == Shape::Type::Path || shape->type() == Shape::Type::Text) {
      return shape->selected() ? CacheType::SelectedPaths : CacheType::NonSelectedPaths;
    }
    if (shape->type() == Shape::Type::Bitmap) return CacheType::Bitmap;
    if (shape->type() == Shape::Type::Group) return CacheType::Group;
  }

  // Categorize the shapes to different cache group
  void addShape(Shape *shape) {
    CacheType group_type = getGroupType(shape);
    if (groups_.isEmpty() || groups_.last().type_ != group_type) {
      groups_.push_back(CacheGroup(group_type));
    }
    groups_.last().shapes_.push_back(shape);
  }

  void end() {
    for (auto &group : groups_) {
      group.merge(screen_rect_);
    }
  }

  QRectF screen_rect_;
  QList<CacheGroup> groups_;
};

#endif