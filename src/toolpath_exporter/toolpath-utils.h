#pragma once

#include <cstdint>
#include <bitset>
#include <vector>
#include <array>
#include <opencv2/core.hpp>
#include <QPolygon>
#include <QPolygonF>
#include <QImage>

using namespace std;

constexpr int WHITE_PIXEL = 255; // should ignored
constexpr int BLACK_PIXEL = 0; // should emit

constexpr int CLIP_FLAG_START = 0b01;
constexpr int CLIP_FLAG_END = 0b10;

struct NestedPolygonF {
  QPolygonF polygon;
  QList<NestedPolygonF> children;
};

using Bitset32 = bitset<32>;
using ByteArray32 = array<unsigned char, 32>;
using namespace std;
tuple<vector<Bitset32>, uint32_t, uint32_t> adjustPrefixSuffixZero(const vector<Bitset32>& src_bit_array, uint32_t padding_dot_cnt);
tuple<vector<ByteArray32>, uint32_t, uint32_t> adjustPrefixSuffixZero(const vector<ByteArray32>& src_bit_array, uint32_t padding_dot_cnt);

QImage imageBinarize(QImage* src, int threshold);
QImage imageTranspose(QImage* img);
bool findMinMaxPixel(QImage* img, int* min_pixel, int* max_pixel);

cv::Mat QPolygonToMat(const QPolygonF& poly);
QPolygon MatIToQPolygon(const cv::Mat& mat);
QPolygonF MatFToQPolygon(const cv::Mat& mat);

bool polygonContainsPolygon(const QPolygonF& outer, const QPolygonF& inner);
void sortByBoundingRect(QList<QPolygonF>& polys);
void traverse(QList<QPolygonF>& polys, const NestedPolygonF& node);

class PathUtils {
 public:
  void setLengthRatio(double x, double y) {
    length_ratio_x_ = x;
    length_ratio_y_ = y;
  }
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
  void sortAndPreprocessPolygons(QList<QPolygonF>& polys);

 private:
  void findChildren(QList<QPolygonF>& polys, NestedPolygonF& parent, int index);
  void sortByDistance(QList<NestedPolygonF>& polys);
  void preprocessPath(QList<NestedPolygonF>& polys);
  void loopCompensate(QPolygonF& poly);
  double getLength(QPointF& start, QPointF& end);

  double length_ratio_x_ = 1;
  double length_ratio_y_ = 1;
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
