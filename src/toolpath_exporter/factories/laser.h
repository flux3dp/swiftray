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

  QVector<QRect> get_iteration_data(int padding_pixel = 0);
  bool fg_iterate_x_pwm(const uchar* data, int l, int r, float y, bool reverse);
  bool fg_iterate_x(const uchar* data, int l, int r, float y, bool reverse);
  bool iterate_x(const uchar* data, int l, int r, float y, bool reverse);

 public:
  LaserBitmapFactory(const FactoryKwargs& kwargs) noexcept;

  void set_default_workspace(int val) override;
  void set_pwm_engraving(bool val) override;
  void add_filled_path(QPainterPath& path,
                       QRectF& bbox,
                       int index = -1) override;
  void generate_task_code(GenerateTaskKwargs kwargs) override;
};
