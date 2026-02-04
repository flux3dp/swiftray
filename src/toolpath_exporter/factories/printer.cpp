#include "printer.h"

void PrinterBitmapFactory::set_preparatory_task_bbox(QRectF bbox) {
  /*
  int x = std::round(mm2px(config_.prespray.x()));
  int y = std::round(mm2px(config_.prespray.y()));
  int w = std::round(mm2px(config_.prespray.width()));
  int h = std::round(mm2px(config_.prespray.height()));
  int prespray_w_px = mm2px(8);
  int x_safe_dist = std::round(mm2px(2));

  int prespray_x, prespray_y, prespray_w, prespray_h;
  int test_x, test_y, test_w, test_h;

  if (w > prespray_w_px + 2 * x_safe_dist) {
    prespray_x = x + (w - prespray_w_px) / 2;
    prespray_w = prespray_w_px;
  } else {
    prespray_x = x + x_safe_dist;
    prespray_w = w - 2 * x_safe_dist;
  }
  test_x = x + x_safe_dist;
  test_w = w - 2 * x_safe_dist;
  if (h > 2 * printing_slice_height) {
    int padding = (h - 2 * printing_slice_height) / 3;
    prespray_y = y + padding;
    prespray_h = printing_slice_height;
    test_y = y + 2 * padding + printing_slice_height;
    test_h = printing_slice_height;
  } else if (h > printing_slice_height) {
    int padding = (h - printing_slice_height) / 2;
    prespray_y = y + padding;
    prespray_h = printing_slice_height;
    test_y = y + padding;
    test_h = printing_slice_height;
  } else {
    prespray_y = y;
    prespray_h = h;
    test_y = y;
    test_h = h;
  }
  return std::make_tuple(QRect(prespray_x, prespray_y, prespray_w, prespray_h),
                         QRect(test_x, test_y, test_w, test_h));
  */
}
