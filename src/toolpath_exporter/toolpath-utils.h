#pragma once

#include "layer.h"
#include "toolpath-exporter-types.h"
#include <QImage>
#include <QPolygonF>
#include <QSet>
#include <opencv2/core.hpp>
#include <array>
#include <bitset>
#include <vector>

std::tuple<std::vector<Bitset32>, uint32_t, uint32_t> adjustPrefixSuffixZero(
    const std::vector<Bitset32>& src_bit_array,
    uint32_t padding_dot_cnt);
std::tuple<std::vector<ByteArray32>, uint32_t, uint32_t> adjustPrefixSuffixZero(
    const std::vector<ByteArray32>& src_bit_array,
    uint32_t padding_dot_cnt);

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
  void sortAndPreprocessPolygons(QVector<QPolygonF>& polys);

 private:
  void findChildren(QVector<QPolygonF>& polys, NestedPolygonF& parent, int index);
  void sortByDistance(QVector<NestedPolygonF>& polys);
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
int get_laser_delay(HardwareType hw_type, int watt);
bool is_printing_module(LayerModule module);
bool is_uv_module(LayerModule module);
PrintingColor get_color(QString hex_color);
InwardRect get_boundary(HardwareType hw_type, LayerModule layer_module);
double get_default_min_padding(
    HardwareType hw_type,
    LayerModule layer_module,
    MachineModules machine_module = MachineModules::NONE);

double get_padding_dist(double min_padding, float speed, float acc);
