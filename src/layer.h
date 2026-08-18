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

  // BS
  double minPower() const;
  bool isOneWayEngraving() const;
  int module() const;
  int ink() const;
  double printingSpeed() const;
  int multipass() const;
  int halftone() const;
  double amDensity() const;
  float printingStrength() const;
  double cRatio() const;
  double mRatio() const;
  double yRatio() const;
  double kRatio() const;
  float smooth() const;
  const QString& rawAmAngleMap() const;
  const QString& rawColorCurvesMap() const;
  int refreshInterval() const;
  int nozzleMode() const;
  double nozzleOffsetX() const;
  double nozzleOffsetY() const;
  float focus() const;
  float focusStep() const;
  double ceZLimit() const;
  int interpolation() const;
  double rightPadding() const;
  int printingTopPadding() const;
  int printingBotPadding() const;
  int uvPrintingRepeat() const;
  int uvCuringAfter() const;
  int uvCuringRepeat() const;
  int uvStrength() const;
  int uvXStep() const;
  int frequency() const;
  int pulseWidth() const;
  double fillInterval() const;
  double fillAngle() const;
  bool fillBidirectional() const;
  bool fillHatch() const;
  int dottingTime() const;
  double wobbleStep() const;
  double wobbleDiameter() const;
  int airAssist() const;
  const QString& rawBBox() const;
  int laserDelay() const;
  int dpmm() const;
  bool isHighQuality() const;
  bool sCurveEnable() const;
  float sCurveA0() const;
  float sCurveAMax() const;
  float sCurveJerk() const;
  // end BS

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
  double min_power_ = 0;
  bool is_one_way_engraving_ = false;
  int module_ = 15;
  // Printing
  int ink_ = 3;
  double printing_speed_ = 60;
  int multipass_ = 1;
  int halftone_ = 1;
  double am_density_ = 2;
  float printing_strength_ = 100;
  double c_ratio_ = 100;
  double m_ratio_ = 100;
  double y_ratio_ = 100;
  double k_ratio_ = 100;
  float smooth_ = 1;
  QString raw_am_angle_map_;
  QString raw_color_curves_map_;
  int refresh_interval_ = 0;
  int nozzle_mode_ = 0;
  double nozzle_offset_x_ = 0;
  double nozzle_offset_y_ = 0;
  // Height / Curve Engraving
  float focus_ = -2;
  float focus_step_ = -2;
  double ce_z_limit_ = 0;
  // UV configs
  int interpolation_ = 1;
  double right_padding_ = 0;
  // px, -1 for unset, overrides the global printing paddings when >= 0
  int printing_top_padding_ = -1;
  int printing_bot_padding_ = -1;
  int uv_printing_repeat_ = 1;
  int uv_curing_after_ = 0;
  int uv_curing_repeat_ = 1;
  int uv_strength_ = 25;
  int uv_x_step_ = 1;
  // Promark
  int frequency_ = 0;
  int pulse_width_ = 0;
  double fill_interval_;
  double fill_angle_;
  bool fill_bidirectional_;
  bool fill_hatch_;
  int dotting_time_ = 0;
  double wobble_step_ = 0;
  double wobble_diameter_ = 0;
  // other
  int air_assist_ = 100;
  QString raw_bbox_;
  int laser_delay_ = 0;
  int dpmm_ = 0;
  bool is_high_quality_ = false;
  bool s_curve_enable_ = false;
  float s_curve_a0_ = 0;
  float s_curve_a_max_ = 0;
  float s_curve_jerk_ = 0;
};

typedef std::shared_ptr<Layer> LayerPtr;
