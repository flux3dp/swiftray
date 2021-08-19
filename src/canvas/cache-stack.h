#pragma once

#include <QDebug>
#include <QList>
#include <shape/path-shape.h>

class GroupShape;

class Document;

class Canvas;

// TODO (Rewrite to support transform dirty or content dirty, transform dirty does not require heavy recalculation)
/**
 \class CacheStack
 \brief The CacheStack class represents a Layer/GroupShape rendering helper that automatically groups children shapes with similar properties and render them in batches instead of one by one.
 */
class CacheStack {
public:
  /**
   \class Cache
   \brief The Cache class represents a groups of shapes with similar properties that can be rendered in a batch painting command
   */
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

    /**
    * Merge all visible shapes in shapes() into one joined path
    * @param globalShape The transform is used to calculate which shapes are inside the visible area
    */
    void merge(const QTransform &global_transform);

    Cache::Type type() const;

    /**
    * Return shapes in this cache batch
    */
    const QList<Shape *> &shapes() const;

    // CacheFragment add shape
    void addShape(Shape *shape);

    /**
    * Paint the joined path with stroke
    * @param painter The QPainter Object
    * @param pen Colored Pen
    */
    void stroke(QPainter *painter, const QPen &pen);

    /**
    * Paint the joined path with fill and cache it into pixmap
    * @param painter The QPainter Object
    * @param pen Colored Brush
    */
    void fill(QPainter *painter, const QPen &pen);

  private:
    /**
    * Fill the joined path
    */
    void cacheFill();

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

  /**
  * Iterate over children in the layer/group and categorize them into different cache groups
  */
  void update();

  int paint(QPainter *painter);

  bool isGroup() const;

  bool isLayer() const;

  const Canvas &canvas() const;

  const Document &document() const;
  
  /**
  * Returns the layer color or group's layer color
  */
  const QColor &color() const;


private:
  /**
   * Categorize the shapes to different cache group
   */
  void addShape(Shape *shape);

  enum class Type {
    Layer,
    Group
  };

  Type type_;
  GroupShape *group_;
  Layer *layer_;
  QTransform global_transform_;
  QList<Cache> caches_;
};

typedef CacheStack::Cache::Type CacheType;