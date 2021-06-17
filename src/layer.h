#include <shape/shape.h>
#include <canvas/cache-stack.h>

#ifndef LAYER_H
#define LAYER_H

class Layer {
public:
  enum class Type {
    Line,
    Fill,
    FillLine,
    Mixed // Compatible mode with Beam Studio, could be deprecated in the future
  };

  Layer(const QColor &color, const QString &name);

  Layer(int new_layer_id);

  Layer();

  ~Layer();

  // Paint the layer with screen rect and dash counter
  int paint(QPainter *painter, int counter) const;

  // Add ShapePtr to children array
  void addShape(const ShapePtr &shape);

  // Return children array
  QList<ShapePtr> &children();

  // Clear all children
  void clear();

  // Clone the children and the layer itself
  shared_ptr<Layer> clone();

  // Remove specific ShapePtr
  void removeShape(const ShapePtr &shape);

  // Remove shapes marked with selected
  void removeSelected();

  // Cache functions
  void flushCache();

  void cache() const;

  void calcPen();

  // Getters:

  QColor color() const;

  QRectF screenRect();

  QString name() const;

  Type type() const;

  bool isVisible() const;

  int repeat() const;

  int speed() const;

  int strength() const;

  // Setters:

  void setColor(const QColor &color);

  void setDiode(int diode_);

  void setHeight(double height);

  void setName(const QString &name);

  void setRepeat(int repeat);

  void setSpeed(int speed);

  void setStrength(int strength);

  void setType(Type type);

  void setVisible(bool visible);

  void setZStep(double zstep);

private:
  QColor color_;
  QString name_;
  QList<ShapePtr> children_;
  Type type_;
  bool is_diode_;
  bool visible_;
  double target_height_;
  double zstep_;
  int repeat_;
  int speed_;
  int strength_;
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
