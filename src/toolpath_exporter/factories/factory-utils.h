#pragma once

#include <QImage>
#include <QRectF>
#include <QVector>

QVector<QRect> get_bounding_boxes(QImage* src,
                                  QRectF dirty_area,
                                  int merge_offset_l,
                                  int merge_offset_r,
                                  int merge_offset_y,
                                  bool enable_segmentation,
                                  int downsample = 1);

// ========= halftone utils =========
float positiveMod(float n, float m);
int apply_color_curve(int inv_val, QVector<int>& color_curve);
int am_halftone(int inv_val,
                int x,
                int y,
                double c,
                double s,
                double dot_radius,
                double dot_spacing,
                double smoother,
                double multiplier);
int fm_halftone(int inv_val, double smoother, double multiplier);