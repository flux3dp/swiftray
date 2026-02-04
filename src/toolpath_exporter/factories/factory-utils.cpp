#include "factory-utils.h"
#include "toolpath_exporter/toolpath-exporter-constants.h"
#include <QDebug>
#include <opencv2/opencv.hpp>

QVector<QRect> get_bounding_boxes(QImage* src,
                                  QRectF dirty_area,
                                  int merge_offset_l,
                                  int merge_offset_r,
                                  int merge_offset_y,
                                  bool enable_segmentation,
                                  int downsample) {
  Q_ASSERT_X(src->format() == QImage::Format_Grayscale8, "ToolpathExporter",
             "Input image for get_bounding_boxes() must be Format_Grayscale8");
  QVector<QRect> res;
  if (!enable_segmentation) {
    int b_left = dirty_area.left() - 1 - merge_offset_l;
    b_left = qMin(qMax(b_left, 0), src->width());
    int b_top = dirty_area.top() - 1;
    b_top = qMin(qMax(b_top, 0), src->height());
    int b_right = dirty_area.right() + 1 + merge_offset_r;
    b_right = qMax(qMin(b_right, src->width()), 0);
    int b_bottom = dirty_area.bottom() + 1;
    b_bottom = qMax(qMin(b_bottom, src->height()), 0);

    if (b_left < b_right && b_top < b_bottom) {
      res.append(QRect(b_left, b_top, b_right - b_left, b_bottom - b_top));
    }
    return res;
  }

  QImage src_i = src->copy();
  src_i.invertPixels();
  int w = src_i.width();
  int h = src_i.height();
  cv::Mat img(h, w, CV_8UC1, const_cast<uchar*>(src_i.bits()),
              static_cast<size_t>(src_i.bytesPerLine()));
  cv::Mat emptyImg(h + merge_offset_y * 2, w, CV_8UC1, cv::Scalar(0));
  std::vector<std::vector<cv::Point>> contours;

  if (downsample > 1) {
    cv::Mat downsampledImg;
    cv::resize(img, downsampledImg, cv::Size(w / downsample, h / downsample), 0,
               0, cv::INTER_AREA);
    cv::findContours(downsampledImg, contours, cv::RETR_EXTERNAL,
                     cv::CHAIN_APPROX_SIMPLE);
  } else {
    cv::findContours(img, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
  }
  qInfo() << "Initial contour count:" << contours.size();

  for (const auto& c : contours) {
    cv::Rect boundingBox = cv::boundingRect(c);
    int x = boundingBox.x;
    int y = boundingBox.y;
    int w = boundingBox.width;
    int h = boundingBox.height;

    if (downsample > 1) {
      x = std::max(x - 1, 0) * downsample;
      y = std::max(y - 1, 0) * downsample;
      w = (w + 2) * downsample;
      h = (h + 2) * downsample;
    }

    // Add merge offset padding
    // Note: y is already offseted by adding extra height to emptyImg
    cv::Point start(x - merge_offset_l, y);
    cv::Point end(x + w + merge_offset_r - 1, y + h + 2 * merge_offset_y - 1);
    cv::rectangle(emptyImg, start, end, cv::Scalar(255), cv::FILLED);
  }
  cv::findContours(emptyImg, contours, cv::RETR_EXTERNAL,
                   cv::CHAIN_APPROX_SIMPLE);

  int lastContourCount = contours.size();
  int safeCount = 0;
  qInfo() << "Start iterations with contour count:" << lastContourCount;
  // Merge all contours
  while (lastContourCount > 1) {
    for (const auto& c : contours) {
      cv::rectangle(emptyImg, cv::boundingRect(c), cv::Scalar(255), cv::FILLED);
    }
    cv::findContours(emptyImg, contours, cv::RETR_EXTERNAL,
                     cv::CHAIN_APPROX_SIMPLE);
    if (contours.size() == lastContourCount) {
      break;
    }

    lastContourCount = contours.size();
    safeCount++;
    if (safeCount > 10) {
      qWarning() << "get_bounding_boxes: safeCount > 10";
      break;
    }
  }

  for (const auto& c : contours) {
    cv::Rect boundingBox = cv::boundingRect(c);
    res.append(QRect(boundingBox.x, boundingBox.y, boundingBox.width,
                     boundingBox.height - 2 * merge_offset_y));
    qDebug() << res.last();
  }
  qInfo() << "Final contour count:" << contours.size() << "after" << safeCount
          << "iterations";

  return res;
}

float positiveMod(float n, float m) {
  float val = std::fmod(n, m);
  if (val < 0) {
    val = std::fmod(val + m, m);
  }
  return val;
}

int apply_color_curve(int inv_val, QVector<int>& color_curve) {
  int q = inv_val / 64;
  int r = inv_val % 64;
  return int((color_curve[q] * (64 - r) + color_curve[q + 1] * r) / 64.0);
}

int am_halftone(int inv_val,
                int x,
                int y,
                double c,
                double s,
                double dot_radius,
                double dot_spacing,
                double smoother,
                double multiplier) {
  if (inv_val == BLACK_PIXEL) return BLACK_PIXEL;
  double dot_size = dot_radius * (pow(inv_val / 255.0, smoother) * multiplier * 1.414);
  double dx = positiveMod((x * c - y * s + dot_radius), dot_spacing) - dot_radius;
  double dy = positiveMod((x * s + y * c + dot_radius), dot_spacing) - dot_radius;
  double d = pow(pow(dx, 2) + pow(dy, 2), 0.5);
  if (d <= dot_size) {
    return WHITE_PIXEL;
  } else {
    return BLACK_PIXEL;
  }
}

int fm_halftone(int inv_val, double smoother, double multiplier) {
  return qMin(int(255 * pow(inv_val / 255.0, smoother) * multiplier), WHITE_PIXEL);
}
