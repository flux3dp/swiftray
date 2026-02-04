#pragma once

#include <QImage>
#include <QRectF>
#include <QVector>

QVector<QRect> get_bounding_boxes(QImage* src,
                                  int merge_offset_x,
                                  int merge_offset_y,
                                  int downsample = 1);

// ========= halftone utils =========
float positiveMod(float n, float m);
int apply_color_curve(int inv_val, QVector<int>& color_curve);
int am_halftone(int inv_val,
                int x,
                int y,
                double am_cos,
                double am_sin,
                double am_dot_r,
                double am_dot_d,
                double halftone_smoother,
                double halftone_multiplier);
int fm_halftone(int inv_val, double halftone_smoother, double halftone_multiplier);