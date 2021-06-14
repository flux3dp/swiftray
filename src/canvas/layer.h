#include <shape/shape.h>

#ifndef LAYER_H
#define LAYER_H

class Layer {
public:
  Layer(QColor color, QString name);

  Layer(int new_layer_id);

  Layer();

  int paint(QPainter *painter, QRectF screen_rect, int counter) const;

  void addShape(ShapePtr shape);

  void removeShape(ShapePtr shape);

  void clear();

  shared_ptr<Layer> clone();

  QList<ShapePtr> &children();

  int repeat() const;

  int speed() const;

  int strength() const;

  QColor color() const;

  QString name() const;

  void setColor(QColor color);

  void setHeight(double height);

  void setName(const QString &name);

  void setSpeed(int speed);

  void setStrength(int strength);

  void setRepeat(int repeat);

  void setDiode(int diode_);

  void setZStep(double zstep);


  bool isVisible() const;

  void setVisible(bool visible);

  void flushCache();

  void cache(QRectF screenRect) const;

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
  mutable bool cache_valid_;
  mutable QPainterPath selected_path_;
  mutable QPainterPath non_selected_path_;
  mutable int displaying_paths_count_;
};

typedef shared_ptr<Layer> LayerPtr;

#endif // LAYER_H
