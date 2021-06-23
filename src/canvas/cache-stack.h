#ifndef CACHE_STACK_H
#define CACHE_STACK_H

#include <QDebug>
#include <QList>
#include <shape/path-shape.h>

class GroupShape;

class Document;

class Canvas;

// Rewrite to support transform dirty or content dirty, transform dirty does not require heavy recalculation
class CacheStack {
public:
  // Cache groups of shapes with similar properties
  class Cache {
  public:
    enum class Type {
      SelectedPaths,
      NonSelectedPaths,
      SelectedFilledPaths, // Compatible mode with mixed layer, could be deprecated
      NonSelectedFilledPaths, // Compatible mode with mixed layer, could be deprecated
      Bitmap,
      Group
    };

    Cache(CacheStack *stack, Type type);

    void merge(const QTransform &global_transform);

    Cache::Type type() const;

    const QList<Shape *> &shapes() const;

    // CacheFragment add shape
    void addShape(Shape *shape);

    void cacheFill();

    // Painting functions
    void stroke(QPainter *painter, const QPen &pen);

    void fill(QPainter *painter, const QPen &pen);

  private:
    /* Main properties */
    QList<Shape *> shapes_;
    CacheStack *stack_;
    Type type_;

    /* Cache properties */
    QRectF bbox_;
    QPixmap cache_pixmap_;
    bool is_fill_cached_;
    QPainterPath joined_path_;
  };

  CacheStack(GroupShape *group);

  CacheStack(Layer *layer);

  void update();

  int paint(QPainter *painter);

  bool isGroup() const;

  bool isLayer() const;

  const Canvas &canvas() const;

  const Document &document() const;

  const QColor &color() const;

  QTransform global_transform_;
  QList<Cache> caches_;

private:
  // Categorize the shapes to different cache group
  void addShape(Shape *shape);

  enum class Type {
    Layer,
    Group
  };

  Type type_;
  GroupShape *group_;
  Layer *layer_;
};

typedef CacheStack::Cache::Type CacheType;

#endif