#pragma once

#include <constants.h>
#include <document.h>
#include <layer.h>
#include <shape/bitmap-shape.h>
#include <shape/group-shape.h>
#include <shape/path-shape.h>
#include <toolpath_exporter/generators/base-generator.h>
#include <QList>
#include <QMutex>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include "toolpath-utils.h"

/**
 * Convert layers convex hull of each elements
 * and export a framing gcode task for Promark
 */
class ConvexHullExporter : public QObject {
  Q_OBJECT

 public:
  ConvexHullExporter(BaseGenerator* generator) noexcept;
  bool convertStack(const QList<LayerPtr>& layers);
  void setWorkAreaSize(QRectF work_area);
  bool isExceedingBoundary() { return exceed_boundary_; }

 Q_SIGNALS:
  void progressChanged(int value);

 public Q_SLOTS:
  void handleCancel();

 private:
  void convertLayer(const LayerPtr& layer);
  QPolygonF convertShape(const ShapePtr& shape);
  QPolygonF convertGroup(const GroupShape* group);
  QPolygonF convertBitmap(const BitmapShape* bmp);
  QPolygonF convertPath(const PathShape* path);
  QPolygonF computeConvexHull(QPolygonF& points);
  void outputLayerGcode();
  void moveTo(QPointF p);

  void onProgressChanged(double value, bool absolute);

  QMutex polygons_mutex_;
  BaseGenerator* gen_;

  QTransform global_transform_;
  QTransform current_transform_;
  QRectF machine_work_area_mm_;
  cv::Mat machine_work_area_mat_;
  int repeat_ = 3;  // repeat time for each hull

  QList<QPolygonF> hulls_;
  bool exceed_boundary_ = false;
  bool cancelled_ = false;
  // ===== Calculate current progress percentage ======
  int total_layer_cnt_ = 1;
  int processed_layer_cnt_ = 0;
  double current_progress_ = 0;  // progress within current layer
  int progress_ = 0;
};
