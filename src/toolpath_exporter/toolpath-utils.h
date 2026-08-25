#pragma once

#include "layer.h"
#include "toolpath-exporter-types.h"
#include <QImage>
#include <QPolygonF>
#include <QSet>
#include <opencv2/core.hpp>
#include <array>
#include <bitset>
#include <optional>
#include <vector>

std::tuple<std::vector<Bitset32>, uint32_t, uint32_t> adjustPrefixSuffixZero(
    const std::vector<Bitset32>& src_bit_array,
    uint32_t padding_dot_cnt);
std::tuple<std::vector<ByteArray32>, uint32_t, uint32_t> adjustPrefixSuffixZero(
    const std::vector<ByteArray32>& src_bit_array,
    uint32_t padding_dot_cnt);

struct LaserTextureParams {
  int mode = 1;                  // 1 = random noise, 2 = angled stripes
  double random_intensity = 30;  // +-% of the pixel's own ink
  double stripe_angle = 45;      // degrees
  double stripe_interval = 0.5;  // mm between stripe lines
  double stripe_intensity = 50;  // % of ink removed on a stripe line
  double pixel_size_x = 0.1;     // mm per px; may differ from y (anisotropic dpmm)
  double pixel_size_y = 0.1;
};
void applyLaserTexture(QImage* src, const LaserTextureParams& params);

QImage imageBinarize(QImage* src, int threshold);
void imageBinarizeARGB32(QImage* src, int threshold);
QImage imageTranspose(QImage* img);
bool findMinMaxPixel(QImage* img, int* min_pixel, int* max_pixel);

cv::Mat QImageToMat(const QImage& img);
QImage MatToQImage(const cv::Mat& mat);

cv::Mat QPolygonToMat(const QPolygonF& poly);
QPolygon MatIToQPolygon(const cv::Mat& mat);
QPolygonF MatFToQPolygon(const cv::Mat& mat);

bool polygonContainsPolygon(const QPolygonF& outer, const QPolygonF& inner);
void sortByBoundingRect(QVector<QPolygonF>& polys);
void traverse(QVector<QPolygonF>& polys, const NestedPolygonF& node);

class PathUtils {
 public:
  void setLoopCompensation(double loop_compensation) {
    loop_compensation_ = loop_compensation;
    should_compensate_ = loop_compensation_ > 0;
  }
  void setClipRect(double top_y, double right_x, double bottom_y, double left_x) {
    top_y_ = top_y;
    right_x_ = right_x;
    bottom_y_ = bottom_y;
    left_x_ = left_x;
    top_border_ = QLineF(left_x_, top_y_, right_x_, top_y_);
    right_border_ = QLineF(right_x_, top_y_, right_x_, bottom_y_);
    bottom_border_ = QLineF(right_x_, bottom_y_, left_x_, bottom_y_);
    left_border_ = QLineF(left_x_, bottom_y_, left_x_, top_y_);
  }
  int clipWorkarea(QPointF* start, QPointF* end);
  void sortAndPreprocessPolygons(QVector<QPolygonF>& polys, bool use_ga = true);

 private:
  void findChildren(QVector<QPolygonF>& polys, NestedPolygonF& parent, int index, bool use_ga);
  void sortByDistance(QVector<NestedPolygonF>& polys, bool use_ga);
  void preprocessPath(QVector<NestedPolygonF>& polys);
  void loopCompensate(QPolygonF& poly);
  double getLength(QPointF& start, QPointF& end);

  bool should_compensate_ = false;
  double loop_compensation_ = 0;
  double top_y_ = 0;
  double right_x_ = 0;
  double bottom_y_ = 0;
  double left_x_ = 0;
  QLineF top_border_;
  QLineF right_border_;
  QLineF bottom_border_;
  QLineF left_border_;
};

QByteArray generate_nozzle_setting_payload(int saturation,
                                           float voltage = NAN,
                                           float pulse_width = NAN);

QSet<CollisionRegions> check_intersection(
    const QList<LayerPtr>& layers,
    HardwareType hardware,
    QSet<LayerModule> all_modules,
    double acc,
    QMap<LayerModule, QPointF> module_offsets,
    double min_printing_padding = 0,
    double min_engraving_padding = 0);

HardwareType model_to_hardware_type(const QString& hw_name);
double get_backlash_compensation(
    HardwareType hw_type,
    double speed,
    MachineModules machine_module = MachineModules::NONE);
float get_laser_delay(HardwareType hw_type, int watt, float speed);
bool is_printing_module(LayerModule module);
bool is_uv_module(LayerModule module);
PrintingColor get_color(QString hex_color);
InwardRect get_boundary(HardwareType hw_type, LayerModule layer_module);
double get_default_min_padding(
    HardwareType hw_type,
    LayerModule layer_module,
    MachineModules machine_module = MachineModules::NONE,
    bool is_high_quality = false);

double get_padding_dist(double min_padding, float speed, float acc);

struct SCurveParameters {
  double a0;
  double a_max;
  double jerk;
};

std::optional<SCurveParameters> get_s_curve_parameters(HardwareType hw_type,
                                                       float speed);

/**
 * Compute the S-curve acceleration padding distance from explicit motion
 * parameters (independent of hardware lookup).
 *
 * @param v_target target velocity in mm/s
 * @param a0 initial acceleration in mm/s^2
 * @param a_max max acceleration in mm/s^2
 * @param jerk jerk in mm/s^3
 * @param v0 initial velocity in mm/s (defaults to 0)
 * @return padding distance in mm
 */
double calculate_s_curve_padding_dist(double v_target, double a0, double a_max,
                                      double jerk, double v0 = 0);

/**
 * Compute the S-curve acceleration padding distance for the given hardware
 * and target speed. Returns NaN if the hardware does not support s-curve.
 *
 * @param hw_type hardware type
 * @param speed in mm/s (target velocity)
 * @param v0 initial velocity in mm/s (defaults to 0)
 * @return padding distance in mm, or NaN if unsupported
 */
double get_s_curve_padding_dist(HardwareType hw_type, float speed, double v0 = 0);
