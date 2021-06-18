#ifndef CACHE_STACK_H
#define CACHE_STACK_H

#include <QDebug>
#include <QList>
#include <shape/path-shape.h>

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

    explicit Cache(Type type);

    void merge(const QTransform &global_transform);

    Cache::Type type() const;

    const QList<Shape *> &shapes() const;

    void addShape(Shape *shape);

    const QPixmap &fillCache(QPainter *painter, QBrush &brush);

    // Path cache
    QPainterPath joined_path_;
  private:
    // Use weak pointer (no lifespan issue here)
    QList<Shape *> shapes_;
    Type type_;
    // Filling pixmap cache (filling is a computation heavy work for complex paths)
    QPixmap cache_pixmap_;
    bool is_fill_cached_;
    QRectF bbox_;
  };

  CacheStack();

  // Set required information for caches
  void begin(const QTransform &global_transform = QTransform());

  // Calculate the cache
  void end();

  // Categorize the shapes to different cache group
  void addShape(Shape *shape);

  int paint(QPainter *painter);

  void setBrush(const QBrush &brush);

  void setPen(const QPen &dash_pen, const QPen &solid_pen);

  void setForceFill(bool force_fill);

  void setForceSelection(bool force_selection);


  QTransform global_transform_;
  QList<Cache> caches_;

private:
  QPen dash_pen_;
  QPen solid_pen_;
  QBrush filling_brush_;
  bool force_fill_;
  bool force_selection_;
};

typedef CacheStack::Cache::Type CacheType;

#endif