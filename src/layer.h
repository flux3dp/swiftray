#pragma once

#include <QDebug>
#include <QMutex>
#include <shape/shape.h>
#include <meta/layer-parameters.h>
#include <parser/mysvg/mysvg-types.h>

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

  void paintUnselected(QPainter *painter, double line_width);

  // Add ShapePtr to children array
  void addShape(const ShapePtr &shape);

  // Return children array
  QList<ShapePtr> &children();

  // Clone the children and the layer itself
  std::shared_ptr<Layer> clone();

  // Remove specific ShapePtr
  void removeShape(const ShapePtr &shape);

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

  int module() const;

  float focus() const;

  float focusStep() const;

  float printingStrength() const;

  double printingSpeed() const;

  int uv() const;

  int halftone() const;

  int multipass() const;

  int ink() const;

  double minPower() const;

  int frequency() const;
  
  int pulseWidth() const;

  double fillInterval() const;

  double fillAngle() const;

  bool fillBidirectional() const;

  bool fillHatch() const;

  int dottingTime() const;

  Document &document();

  /** Setters **/

  void setColor(const QColor &color);

  void setName(const QString &name);

  void setParameterIndex(int parameter_index);

  void setType(Type type);

  void setLocked(bool isLocked);

  void setVisible(bool visible);

  void setDocument(Document *doc);

  void setParameters(const LayerParameters &params);

  void setParameters(const MySVG::BeamLayerConfig &params);

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

  /** BS properties **/
  int module_ = 0;
  // new config method for target_height_ and step_height_
  float focus_ = -2;
  float focus_step_ = -2;
  // printing
  float printing_strength_ = 100;
  double printing_speed_ = 60;
  int uv_ = 0;
  int halftone_ = 1;
  int multipass_ = 1;
  int ink_ = 3;
  // pwm
  double min_power_ = 0;
  // fiber
  int frequency_ = 0;
  int pulse_width_ = 0;
  // filling
  double fill_interval_;
  double fill_angle_;
  bool fill_bidirectional_;
  bool fill_hatch_;
  // Promark gradient
  int dotting_time_ = 0;
};

typedef std::shared_ptr<Layer> LayerPtr;
