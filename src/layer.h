#include <shape/shape.h>
#include <canvas/cache-stack.h>

#ifndef LAYER_H
#define LAYER_H

class Layer {
public:
  Layer(QColor color, QString name);

  Layer(int new_layer_id);

  Layer();

  ~Layer();

  // Paint the layer with screen rect and dash counter
  int paint(QPainter *painter, QRectF screen_rect, int counter) const;

  // Add ShapePtr to children array
  void addShape(ShapePtr shape);

  // Return children array
  QList<ShapePtr> &children();

  // Clear all children
  void clear();

  // Clone the children and the layer itself
  shared_ptr<Layer> clone();

  // Remove specific ShapePtr
  void removeShape(ShapePtr shape);

  // Remove shapes marked with selected
  void removeSelected();

  // Cache functions
  void flushCache();

  void cache(QRectF screenRect) const;

  void calcPen();

  // Getters:
  int repeat() const;

  int speed() const;

  int strength() const;

  QColor color() const;

  QString name() const;

  QRectF screenRect();

  bool isVisible() const;

  // Setters:
  void setColor(QColor color);

  void setHeight(double height);

  void setName(const QString &name);

  void setSpeed(int speed);

  void setStrength(int strength);

  void setRepeat(int repeat);

  void setDiode(int diode_);

  void setZStep(double zstep);

  void setVisible(bool visible);

private:
  QColor color_;
  QString name_;
  QList<ShapePtr> children_;
  int strength_;
  int speed_;
  double zstep_;
  bool diode_;
  int repeat_;
  double height_;
  bool visible_;
  // Cache properties
  mutable bool cache_valid_;
  mutable CacheStack cache_stack_;
  mutable QRectF screen_rect_;
  // Pen properties
  mutable QPen dash_pen_;
  QPen solid_pen_;
};

typedef shared_ptr<Layer> LayerPtr;

#endif // LAYER_H
