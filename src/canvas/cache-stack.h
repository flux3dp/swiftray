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
      SelectedFilledPaths, // Compatible mode with mixed layer, could be deprecated
      NonSelectedFilledPaths, // Compatible mode with mixed layer, could be deprecated
      Bitmap,
      Group
    };

    explicit Cache(Type type);

    void merge(const QTransform &base_transform);

    Cache::Type type() const;

    const QList<Shape *> shapes() const;

    const QPixmap &fillCache(QPainter *painter, QBrush brush);

    // Use weak pointer (no lifespan issue here)
    QList<Shape *> shapes_;
    Type type_;
    QPainterPath joined_path_;
    QPixmap cache_pixmap_;
    bool is_fill_cached_;
    QRectF bbox_;
  };

  // Set required information for caches
  void begin(const QTransform &base_transform = QTransform());

  // Calculate the cache
  void end();

  // Categorize the shapes to different cache group
  void addShape(Shape *shape);

  int paint(QPainter *painter);

  void setBrush(const QBrush &brush) {
    filling_brush_ = brush;
  }

  void setPen(const QPen &selected_pen, const QPen &nonselected_pen) {
    selected_pen_ = selected_pen;
    nonselected_pen_ = nonselected_pen;
  }

  void setForceFill(bool force_fill) {
    force_fill_ = force_fill;
  }


  QTransform base_transform_;
  QList<Cache> caches_;

private:
  QPen selected_pen_;
  QPen nonselected_pen_;
  QBrush filling_brush_;
  bool force_fill_;
};

typedef CacheStack::Cache::Type CacheType;

#endif