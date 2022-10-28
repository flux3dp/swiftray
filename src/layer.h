#pragma once

#include <QMutex>
#include <shape/shape.h>
#include <canvas/cache-stack.h>

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
  std::shared_ptr<Layer> clone();

  // Remove specific ShapePtr
  void removeShape(const ShapePtr &shape);

  // Cache functions
  void flushCache();

  /** Getters **/

  const QColor &color() const;

  const QString &name() const;

  Type type() const;

  bool isLocked() const;

  bool isVisible() const;

  bool isUseDiode() const;

  int repeat() const;

  double speed() const;

  double power() const;

  double xBacklash() const;

  int parameterIndex() const;

  double stepHeight() const;

  double targetHeight() const;

  Document &document();


  /** Setters **/

  void setColor(const QColor &color);

  void setUseDiode(bool is_diode);

  void setTargetHeight(double height);

  void setName(const QString &name);

  void setRepeat(int repeat);

  void setSpeed(double speed);

  void setStrength(double strength);

  void setXBacklash(double x_backlash);

  void setParameterIndex(int parameter_index);

  void setType(Type type);

  void setLocked(bool isLocked);

  void setVisible(bool visible);

  void setStepHeight(double step_height);

  void setDocument(Document *doc);

  //void setLayerCounter(int i);

  friend class DocumentSerializer;

private:
  /** Main properties **/
  Document *document_;
  QColor color_;
  QString name_;
  QList<ShapePtr> children_;
  QMutex children_mutex_;
  Type type_;
  bool use_diode_;
  bool is_locked_;
  bool is_visible_;
  double target_height_;
  double step_height_;
  int repeat_;
  double speed_;
  double power_;
  double x_backlash_ = 0;
  int parameter_index_;

  /** Main properties **/
  mutable bool cache_valid_;
  mutable std::unique_ptr<CacheStack> cache_;
};

typedef std::shared_ptr<Layer> LayerPtr;
