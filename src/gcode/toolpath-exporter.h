#pragma once

#include <QList>
#include <QPainter>
#include <layer.h>
#include <shape/bitmap-shape.h>
#include <shape/path-shape.h>
#include <shape/group-shape.h>
#include <gcode/generators/base-generator.h>
#include <document.h>

#include <QImage>
#include <QTime>

class ToolpathExporter {
public:
  ToolpathExporter(BaseGenerator *generator) noexcept;

  void convertStack(const QList<LayerPtr> &layers);

  void setDPMM(qreal new_dpmm) { dpmm_ = new_dpmm; }
  void setWorkAreaSize(QSizeF work_area_size) { machine_work_area_size_ = work_area_size; }

private:
  void convertLayer(const LayerPtr &layer);

  void convertShape(const ShapePtr &shape);

  void convertGroup(const GroupShape *group);

  void convertBitmap(const BitmapShape *bmp);

  void convertPath(const PathShape *path);

  void sortPolygons();

  void outputLayerGcode();

  void outputLayerPathGcode();

  void outputLayerBitmapGcode();

  inline void moveTo(QPointF&& dest, int speed, int power);
  inline void moveTo(const QPointF& dest, int speed, int power);

  bool rasterBitmapRowHighSpeed(unsigned char *data, float global_coord_y, bool reverse, QPointF offset);

  bool rasterBitmapRow(unsigned char *data, qreal real_y_pos, int row_pixel_cnt, bool reverse, QPointF offset);

  QImage imageBinarize(QImage src, int threshold);

  QTransform global_transform_;
  //QList<ShapePtr> layer_elements_;
  QList<QPolygonF> layer_polygons_; // place the unfilled path geometry
  QPixmap layer_bitmap_;            // place the filled geometry & image (excluding unfilled path)
  LayerPtr current_layer_;
  std::unique_ptr<QPainter> layer_painter_;

  BaseGenerator *gen_;
  float dpmm_;
  float travel_speed_;
  QSizeF machine_work_area_size_; // Work area in real world coordinate (in unit of mm)
  QRectF bitmap_dirty_area_; // In canvas unit (not in real world mm unit)
  QSizeF canvas_size_;       // In canvas unit (not in real world mm unit)
  const qreal canvas_mm_ratio_ = 10.0; // Currently 10 unit in canvas = 1 mm in real world
                                       // TBD: Calculate this ratio by (canvas_size_ / machine_work_area_size_)

  QPointF current_pos_; // in unit of mm
};