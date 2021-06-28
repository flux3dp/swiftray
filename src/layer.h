#include <shape/shape.h>
#include <canvas/cache-stack.h>

#ifndef LAYER_H
#define LAYER_H

class Document;

class DocumentSerializer;

class Layer {
public:
  enum class Type {
    Line,
    Fill,
    FillLine,
    Mixed // Compatible mode with Beam Studio, could be deprecated in the future
  };

  /** Constructors **/

  Layer(Document *doc, const QColor &color, const QString &name);

  Layer(Document *doc, int layer_id);

  Layer(Document *doc);

  Layer();


  ~Layer();

  int paint(QPainter *painter) const;

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

  /** Getters **/

  const QColor &color() const;

  const QString &name() const;

  Type type() const;

  bool isVisible() const;

  bool isUseDiode() const;

  int repeat() const;

  int speed() const;

  int power() const;

  double stepHeight() const;

  double targetHeight() const;

  Document &document();


  /** Setters **/

  void setColor(const QColor &color);

  void setUseDiode(bool is_diode);

  void setTargetHeight(double height);

  void setName(const QString &name);

  void setRepeat(int repeat);

  void setSpeed(int speed);

  void setStrength(int strength);

  void setType(Type type);

  void setVisible(bool visible);

  void setStepHeight(double step_height);

  void setDocument(Document *doc);

  void setLayerCounter(int i);

  friend class DocumentSerializer;

private:
  /** Main properties **/
  Document *document_;
  QColor color_;
  QString name_;
  QList<ShapePtr> children_;
  Type type_;
  bool use_diode_;
  bool is_visible_;
  double target_height_;
  double step_height_;
  int repeat_;
  int speed_;
  int power_;

  /** Main properties **/
  mutable bool cache_valid_;
  mutable unique_ptr<CacheStack> cache_;
};

typedef shared_ptr<Layer> LayerPtr;

#endif // LAYER_H
