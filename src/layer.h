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

  explicit Layer(int new_layer_id);

  Layer();

  ~Layer();

  // Paint the layer with screen rect and dash dash_counter_
  int paint(QPainter *painter, int counter) const;

  // Add ShapePtr to children array
  void addShape(const ShapePtr &shape);

  // Return children array
  QList<ShapePtr> &children();

  // Clone the children and the layer itself
  shared_ptr<Layer> clone();

  // Remove specific ShapePtr
  void removeShape(const ShapePtr &shape);

  // Cache functions
  void flushCache();

  void cache() const;

  void calcPen();

  // Getters:

  const QColor &color() const;

  const QString &name() const;

  Type type() const;

  bool isVisible() const;

  bool isDiode() const;

  int repeat() const;

  int speed() const;

  int strength() const;

  double stepHeight() const;

  double targetHeight() const;


  // Setters:

  void setColor(const QColor &color);

  void setDiode(bool is_diode);

  void setTargetHeight(double height);

  void setName(const QString &name);

  void setRepeat(int repeat);

  void setSpeed(int speed);

  void setStrength(int strength);

  void setType(Type type);

  void setVisible(bool visible);

  void setStepHeight(double step_height);

  static QList<QColor> DefaultColors;
private:
  QColor color_;
  QString name_;
  QList<ShapePtr> children_;
  Type type_;
  bool is_diode_;
  bool is_visible_;
  double target_height_;
  double step_height_;
  int repeat_;
  int speed_;
  int strength_;
  // Cache properties
  mutable bool cache_valid_;
  mutable CacheStack cache_stack_;
  // Pen properties
  mutable QPen dash_pen_;
  QPen solid_pen_;
};

typedef shared_ptr<Layer> LayerPtr;

#endif // LAYER_H
