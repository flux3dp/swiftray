#pragma once

#include <QList>
#include <QPainter>
#include <layer.h>
#include <shape/bitmap-shape.h>
#include <shape/path-shape.h>
#include <shape/group-shape.h>
#include <gcode/generators/base-generator.h>

class ToolpathExporter {
public:
  ToolpathExporter(BaseGenerator *generator) noexcept;

  void convertStack(const QList<LayerPtr> &layers);

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

  bool rasterBitmapRowHighSpeed(unsigned char *data, float global_coord_y, bool reverse, QPointF offset);

  bool rasterBitmapRow(unsigned char *data, float global_coord_y, bool reverse, QPointF offset);

  QTransform global_transform_;
  QList<ShapePtr> layer_elements_;
  QList<QPolygonF> layer_polygons_;
  QPixmap layer_bitmap_;
  LayerPtr current_layer_;
  unique_ptr<QPainter> layer_painter_;
  BaseGenerator *gen_;
  float dpmm_;
  float travel_speed_;
  QRectF bitmap_dirty_area_;
};