#pragma once

#include "base-factory.h"

class LaserBitmapFactory : public BaseBitmapFactory {
 private:
  const int pwm_threshold = 254;
  int fg_pwm_limit = 0;
  bool pwm_engraving = false;
  float speed;
  double backlash;
  double pwm_scale;
  bool mock_fast_gradient;
  double padding_dist;
  int padding_px;

  QVector<QRect> get_iteration_data(int padding_pixel = 0,
                                    bool reverse_y = false);
  bool fg_iterate_x_pwm(const uchar* data, int l, int r, float y, bool reverse);
  bool fg_iterate_x(const uchar* data, int l, int r, float y, bool reverse);
  bool iterate_x(const uchar* data, int l, int r, float y, bool reverse);
  // Dev fluence variant of iterate_x: emit each dark run through fluence_.
  bool iterate_x_fluence(const uchar* data, int l, int r, float y, bool reverse);

  using ScanMethod =
      bool (LaserBitmapFactory::*)(const uchar*, int, int, float, bool);
  // Engrave the pixels inside `region` (px) using `method`, far-side-first when
  // !reverse_y. Shared by the whole-raster and per-block emission paths. `pass`
  // is the cross-pass index; it seeds the serpentine phase so runs alternate
  // direction between passes.
  void iterate_region(const QRect& region,
                      bool reverse_y,
                      ScanMethod method,
                      QImage* src_bitmap,
                      int pass = 0);
  // Convert a block region (mm) to a pixel rect, clamped so adjacent blocks tile
  // without overlap.
  QRect block_region_to_px(const QRectF& region_mm) const;

 public:
  LaserBitmapFactory(const FactoryKwargs& kwargs) noexcept;

  void set_default_workspace(int val) override;
  void set_pwm_engraving(bool val) override;
  void add_filled_path(QPainterPath& path,
                       QRectF& bbox,
                       int index = -1) override;
  void generate_task_code(GenerateTaskKwargs kwargs) override;
};
