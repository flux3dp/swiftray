#ifndef CACHE_STACK_H
#define CACHE_STACK_H

#include <QDebug>
#include <QList>
#include <shape/path-shape.h>

class CacheStack {
public:
  // Cache groups of shapes with similar properties
  class Cache {
  public:
    enum class Type {
      SelectedPaths,
      NonSelectedPaths,
      FilledPaths,
      Bitmap,
      Group
    };

    explicit Cache(Type type);

    void merge(const QTransform &base_transform);

    void paint(QPainter *painter);

    Cache::Type type() const;

    const QList<Shape *> shapes() const;

    // Use weak pointer (no lifespan issue here)
    QList<Shape *> shapes_;
    Type type_;
    QPainterPath joined_path_;
  };

  // Set required information for caches
  void begin(const QTransform &base_transform = QTransform());

  // Calculate the cache
  void end();

  // Categorize the shapes to different cache group
  void addShape(Shape *shape);

  QTransform base_transform_;
  QList<Cache> caches_;
};

typedef CacheStack::Cache::Type CacheType;

#endif