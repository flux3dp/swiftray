#include <toolpath_exporter/toolpath-exporter.h>
#include "toolpath-exporter-constants.h"
#include <QElapsedTimer>
#include <QDebug>
#include <QVector2D>
#include <QVector3D>
#include <QtMath>
#include <QProgressDialog>
#include <QCoreApplication>
#include <QPainter>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <map>
#include <queue>
#include <limits>
#include <constants.h>

namespace {
/**
 * @brief One horizontal scan line per row of @p bbox, ending at the exclusive
 *        right edge since rasterBitmap*() samples at pixel centers.
 */
QList<QLine> makeRasterLines(const QRect& bbox) {
  const RectBorders borders = getRectBorders(bbox);
  QList<QLine> raster_lines;
  raster_lines.reserve(bbox.height());
  for (int y = borders.top; y < borders.bottom_exclusive; y++) {
    raster_lines.push_back(
        QLine{borders.left, y, borders.right_exclusive, y});
  }
  return raster_lines;
}

// Fallbacks when neither the placeholder rect nor the layer provides a value.
constexpr double kDefaultStlLayerHeightMm = 0.1;
constexpr double kDefaultStlPointSpacingMm = 0.1;
constexpr qint64 kMaxPhotoSampleCount = 10000000;
}  // namespace

// TODO: Fix ToolpathExporter for non-Promark machines
ToolpathExporter::ToolpathExporter(BaseGenerator *generator, qreal dpmm, double travel_speed, QPointF end_point, PaddingType padding_type, QTransform move_translate, bool is_promark) noexcept :
 gen_(generator), dpmm_(dpmm), padding_type_(padding_type), travel_speed_(travel_speed), end_point_(end_point), move_translate_(move_translate), is_promark_(is_promark) {}

void ToolpathExporter::parseParam(QJsonObject param) {
  qInfo() << "Parsing parameters from JSON object:" << param;
  gen_->addComment("CONFIG RESET");
  if (param.contains("is_uv_light")) {
    int is_uv_light = param["is_uv_light"].toInt();
    gen_->addComment(QString("CONFIG UV=%1").arg(is_uv_light));
  }
  if (param.contains("jump_speed")) {
    double jump_speed = param["jump_speed"].toDouble();
    gen_->addComment(QString("CONFIG JUMP_SPEED=%1").arg(jump_speed));
  }
  if (param.contains("laser_on_delay")) {
    int laser_on_delay = param["laser_on_delay"].toInt();
    gen_->addComment(QString("CONFIG LASER_ON_DELAY=%1").arg(laser_on_delay));
  }
  if (param.contains("laser_off_delay")) {
    uint32_t laser_off_delay = param["laser_off_delay"].toInt();
    gen_->addComment(QString("CONFIG LASER_OFF_DELAY=%1").arg(laser_off_delay));
  }
  if (param.contains("marking_delay")) {
    uint32_t marking_delay = param["marking_delay"].toInt();
    gen_->addComment(QString("CONFIG MARKING_DELAY=%1").arg(marking_delay));
  }
  if (param.contains("corner_delay")) {
    uint32_t corner_delay = param["corner_delay"].toInt();
    gen_->addComment(QString("CONFIG CORNER_DELAY=%1").arg(corner_delay));
  }
  if (param.contains("jump_delay_min")) {
    uint32_t jump_delay_min = param["jump_delay_min"].toInt();
    gen_->addComment(QString("CONFIG JUMP_DELAY_MIN=%1").arg(jump_delay_min));
  }
  if (param.contains("jump_delay_max")) {
    uint32_t jump_delay_max = param["jump_delay_max"].toInt();
    gen_->addComment(QString("CONFIG JUMP_DELAY_MAX=%1").arg(jump_delay_max));
  }
  if (param.contains("first_pulse_killer_enabled")) {
    const int enabled = param["first_pulse_killer_enabled"].toBool() ? 1 : 0;
    gen_->addComment(QString("CONFIG FIRST_PULSE_KILLER_ENABLED=%1").arg(enabled));
  }
  // Basic axial refraction is always applied. n=1 keeps the original Z plane unchanged.
  RefractionParams refraction = refraction_.params();
  if (param.contains("refractive_index")) {
    refraction.refractive_index = param["refractive_index"].toDouble();
  }
  if (param.contains("material_height")) {
    refraction.material_height_mm = param["material_height"].toDouble();
  }
  refraction_.setParams(refraction);

  material_min_z_mm_ = param.contains("material_min_z")
                           ? param["material_min_z"].toDouble()
                           : 0.0;
  material_max_z_mm_ = param.contains("material_max_z")
                           ? param["material_max_z"].toDouble()
                           : refraction.material_height_mm;
  if (!std::isfinite(material_min_z_mm_)) {
    qWarning() << "[Export] material_min_z is not finite; using 0";
    material_min_z_mm_ = 0.0;
  }
  if (!std::isfinite(material_max_z_mm_)) {
    qWarning() << "[Export] material_max_z is not finite; using material_height";
    material_max_z_mm_ = refraction.material_height_mm;
  }
  if (material_max_z_mm_ < material_min_z_mm_) {
    qWarning() << "[Export] empty STL material Z range" << material_min_z_mm_ << "to"
               << material_max_z_mm_;
  }
  is_path_preview_ = param["shouldMockFastGradient"].toBool();
  qInfo() << "is_path_preview_" << is_path_preview_;
}

void ToolpathExporter::setDpmm(qreal dpmm) {
  if (dpmm == dpmm_ || dpmm <= 0) return;
  bool need_bigger_canvas = dpmm > dpmm_;
  dpmm_ = dpmm;
  setLoopCompensation(compensation_mm_);
  setWorkAreaSize(machine_work_area_mm_);
  resolution_scale_ = dpmm_ / canvas_mm_ratio_;
  resolution_scale_transform_ = QTransform::fromScale(resolution_scale_, resolution_scale_);
  global_transform_ = move_translate_ * resolution_scale_transform_;
  canvas_width_ = machine_work_area_mm_.width() * dpmm_;
  canvas_height_ = machine_work_area_mm_.height() * dpmm_;
  canvas_size_ = QSizeF(canvas_width_, canvas_height_);
  double eps = 1e-4;
  canvas_clip_path_.clear();
  canvas_clip_path_.addRect(eps, eps, canvas_width_ - eps * 2, canvas_height_ - eps * 2);
  qInfo() << "Layer dpmm: " << dpmm_ << "Canvas size: " << canvas_size_;
  if (need_bigger_canvas) {
    layer_bitmaps_.clear();
    bitmap_dirty_areas_.clear();
    for (int i = BitmapHandlerType::NormalMode; i < BitmapHandlerType::PwmMode; ++i) {
      layer_bitmaps_.append(QPixmap(QSize(canvas_width_, canvas_height_)));
      layer_bitmaps_[i].fill(Qt::white);
      bitmap_dirty_areas_.append(QRectF());
    }
  }
}

/**
 * @brief Convert all visible layers in the document into gcode (result is stored in gcode generator)
 * @param layers MUST contain at least one layer
 * @retval true if completed,
 *         false if canceled or error occurred
 */
bool ToolpathExporter::convertStack(const QList<LayerPtr> &layers, bool is_high_speed, bool start_with_home) {
  qInfo() << "[Export] Start converting stack with layers" << layers.count();
  is_high_speed_ = is_high_speed;
  progress_ = 0;
  QElapsedTimer t;
  t.start();
  // Initial Setup
  if (!gen_->isRotaryMode()) {
    gen_->disableRotary();
  }
  gen_->setRedLight(is_contour_);
  gen_->turnOffLaser(); // M5
  if(start_with_home) {
    gen_->home();
  }
  gen_->useAbsolutePositioning();
  if (gen_->isRotaryMode()) {
    gen_->enableRotary();
    if (is_promark_) {
      // Make sure cmd list is opened
      gen_->turnOnLaser();
      gen_->homeRotary();
    }
  }
  gen_->setWorkarea(machine_work_area_mm_);

  // Start Parsing Layers
  qInfo() << "[Export] Start parsing layers";
  Q_ASSERT_X(!layers.empty(), "ToolpathExporter", "Must input at least one layer");
  LayerPtr first_layer = layers.at(0);
  qInfo() << "[Export] First layer: " << first_layer->name() << " " << first_layer.get(); 
  qInfo() << "[Export] Document: " << &first_layer->document();
  // Setup dpmm with compensation_mm_ & machine_work_area_mm_
  qreal default_dpmm = dpmm_;
  dpmm_ = 0;
  setDpmm(default_dpmm);

  processed_layer_cnt_ = 0;
  // Progress is measured in exporter passes. A line / "-filled" pair is converted as one pass,
  // and invisible layers do not consume conversion time.
  total_layer_cnt_ = 0;
  for (int index = layers.size() - 1; index >= 0; --index) {
    const LayerPtr &layer = layers[index];
    if (!layer->isVisible()) continue;
    ++total_layer_cnt_;
    if (index > 0 && layer->type() == Layer::Type::Line &&
        layers[index - 1]->isVisible() && layers[index - 1]->type() != Layer::Type::Line &&
        layer->name() + "-filled" == layers[index - 1]->name()) {
      --index;
    }
  }
  total_layer_cnt_ = std::max(1, total_layer_cnt_);
  for (auto layer_rit = layers.crbegin(); layer_rit != layers.crend(); layer_rit++) {
    if ((*layer_rit)->isVisible()) {
      qInfo() << "[Export] Output layer: " << (*layer_rit)->name();
      total_repeat_times_ = (*layer_rit)->repeat();
      if (is_contour_ && total_repeat_times_ > 1) {
        total_repeat_times_ = 1;
      }
      float focus = is_contour_ ? 0 : (*layer_rit)->focus();
      int focus_dir = (*layer_rit)->focusRev() ? -1 : 1;
      float focus_step = is_contour_ ? 0 : (*layer_rit)->focusStep();
      int focus_step_dir = (*layer_rit)->focusStepRev() ? -1 : 1;
      float total_move = 0;
      LayerPtr current_layer_ = *layer_rit;
      LayerPtr current_layer_2_ = nullptr;
      layer_rit++;
      if (layer_rit != layers.crend() &&
          current_layer_->type() == Layer::Type::Line &&
          (*layer_rit)->type() != Layer::Type::Line &&
          current_layer_->name() + "-filled" == (*layer_rit)->name()) {
        // Swiftray create two layers for line + filled path, handle them together
        current_layer_2_ = *layer_rit;
        qInfo() << "[Export] Handle layers together" << current_layer_->name() << current_layer_2_->name();
        // The two halves come from ONE canvas layer, so the parser gives them the same config
        // (my_qsvg_handler_qt6.cpp, layer_config_map_[title + "-filled"] = layer_config_map_[title]).
        // The STL pass relies on that: it engraves both halves under the second layer's parameters.
        if (current_layer_2_->speed() != current_layer_->speed() ||
            current_layer_2_->power() != current_layer_->power()) {
          qWarning() << "[Export] Paired layers have different parameters, speed"
                     << current_layer_->speed() << current_layer_2_->speed() << "power"
                     << current_layer_->power() << current_layer_2_->power()
                     << "-- STL objects of both halves will use the latter";
        }
      } else {
        layer_rit--;
        current_layer_2_ = nullptr;
      }
      for (processed_repeat_times_ = 0; processed_repeat_times_ < total_repeat_times_; processed_repeat_times_++) {
        if (processed_repeat_times_ == 0) {
          if (focus > 0) {
            // Make sure cmd list is opened
            gen_->turnOnLaser();
            gen_->moveZ(-focus * focus_dir);
            total_move += focus * focus_dir;
          }
        } else if (focus_step > 0) {
          // Make sure cmd list is opened
          gen_->turnOnLaser();
          gen_->moveZ(-focus_step * focus_step_dir);
          total_move += focus_step * focus_step_dir;
        }
        convertLayer(current_layer_, current_layer_2_ != nullptr);
        if (cancelled_) break;
        if (current_layer_2_) {
          convertLayer(current_layer_2_);
          if (cancelled_) break;
        }
      }
      // Cancellation is immediate. In particular, do not append a compensating Z move that
      // returns focus to the pre-conversion position; the incomplete output is discarded.
      if (!cancelled_ && total_move != 0) {
        gen_->moveZ(total_move);
      }
    }
    if (this->cancelled_) {
      break;
    }
    total_repeat_times_ = processed_repeat_times_ = 1;
    onProgressChanged(0, true);
    processed_layer_cnt_++;
  }

  if (this->cancelled_) {
    return false;
  }

  if (is_promark_ && gen_->isRotaryMode()) {
    gen_->homeRotary();
  }
  gen_->finishProgramFlow();
  
  if (this->cancelled_) {
    return false;
  }

  // Post cmds
  if (!is_promark_) {
    gen_->home();
    moveTo(end_point_, travel_speed_, 0, 0);
  }
  qInfo() << "[Export] Took " << t.elapsed() << " milliseconds";
  return true;
}


/**
 * @brief Convert Shapes on canvas layer into scaled polygons/layer_bitmap (by resolution)
 *        e.g. if dpmm = 10 (Mid res), canvas_mm = 10 -> scale by 1
 *             if dpmm = 20 (High res), canvas_mm = 10 -> scale by 2
 * @param layer
 */
void ToolpathExporter::convertLayer(const LayerPtr &layer, bool stl_paired) {
  // Reset context states for the layer
  setDpmm(layer->dpmm());
  // TODO (Use layer_painter to manage transform over different sub objects)
  global_transform_ = QTransform() * move_translate_ * resolution_scale_transform_;
  polygons_mutex_.lock();
  layer_polygons_.clear();
  layer_filled_polygons_.clear();
  polygons_mutex_.unlock();
  // Keep what the first half of a pair collected, this call is the second half.
  const bool is_paired_second = stl_output_deferred_;
  if (!is_paired_second) layer_stl_placements_.clear();
  stl_output_deferred_ = stl_paired;
  with_image_ = false;
  for (int i = 0; i < 5; i++) {
    element_cnt_[i] = 0;
  }
  // bitmap_dirty_areas_ records only the extent; stale pixels inside it bleed
  // into this layer. Snap outward with a margin, since drawImage() can paint a
  // pixel beyond the recorded bbox.
  for (int i = BitmapHandlerType::NormalMode; i < BitmapHandlerType::PwmMode; ++i) {
    if (!bitmap_dirty_areas_[i].isEmpty()) {
      QPainter painter(&layer_bitmaps_[i]);
      painter.fillRect(
          bitmap_dirty_areas_[i].toAlignedRect().adjusted(-1, -1, 1, 1),
          Qt::white);
    }
    bitmap_dirty_areas_[i] = QRectF();
  }
  current_layer_ = layer;
  if (!is_contour_ && layer->frequency() != 0) {
    // Make sure cmd list is opened
    gen_->turnOnLaser();
    gen_->setFrequency(layer->frequency());
  }
  if (!is_contour_ && layer->pulseWidth() != 0) {
    // Make sure cmd list is opened
    gen_->turnOnLaser();
    gen_->setPulseWidth(layer->pulseWidth());
  }
  if (!is_contour_ && layer->qPulseWidth() != 0) {
    // Make sure cmd list is opened
    gen_->turnOnLaser();
    gen_->setQPulseWidth(layer->qPulseWidth());
  }
  // Iterate through all shapes in the layer
  for (auto &shape : layer->children()) {
    convertShape(shape);
  }
  sortPolygons();
  // A mixed canvas layer is represented as two exporter layers. Reserve the first quarter for
  // its first half, then let the second half use the remaining range (including STL processing).
  // This keeps progress monotonic and prevents a deferred first half from reporting 100% before
  // the expensive STL pass starts.
  if (stl_paired) {
    outputLayerGcode(0.0, 0.25);
  } else if (is_paired_second) {
    outputLayerGcode(0.25, 0.75);
  } else {
    outputLayerGcode();
  }
}

void ToolpathExporter::convertShape(const ShapePtr &shape) {
  switch (shape->type()) {
    case Shape::Type::Group:
      convertGroup(dynamic_cast<GroupShape *>(shape.get()));
      break;

    case Shape::Type::Bitmap:
      with_image_ = true;
      convertBitmap(dynamic_cast<BitmapShape *>(shape.get()));
      break;

    case Shape::Type::Path:
    case Shape::Type::Text:
      convertPath(dynamic_cast<PathShape *>(shape.get()));
      break;

    default:
      break;
  }
}

void ToolpathExporter::convertGroup(const GroupShape *group) {
  global_transform_ = group->globalTransform() * move_translate_ * resolution_scale_transform_;
  for (auto &shape : group->children()) {
    convertShape(shape);
  }
  global_transform_ = QTransform() * move_translate_ * resolution_scale_transform_;
}

// TODO: handling depth mode
/**
 * @brief Draw the image shape on canvas onto layer pixmap
 * @param bmp
 */
void ToolpathExporter::convertBitmap(BitmapShape *bmp) {
  if (is_contour_) {
    polygons_mutex_.lock();
    QPainterPath transformed_bbox;
    transformed_bbox.addPolygon(bmp->rotatedBBox());
    transformed_bbox = transformed_bbox.intersected(canvas_clip_path_);
    layer_polygons_.append(transformed_bbox.toSubpathPolygons());
    polygons_mutex_.unlock();
    return;
  }
  if (bmp->isStlPhoto()) {
    const QRectF projected_bounds = global_transform_.mapRect(bmp->boundingRect());
    const QRectF boundary = resolution_scale_transform_.mapRect(machine_work_area_mm_);
    if (!exceed_boundary_ && !boundary.contains(projected_bounds)) exceed_boundary_ = true;

    layer_stl_placements_.append(
        StlPlacementJob{bmp->stlPlacement(), StlEngraveKind::kDot, bmp});
    return;
  }
  QRectF new_dirty_area = global_transform_.mapRect(bmp->boundingRect());
  QTransform transform = bmp->transform() * global_transform_;
  if (bmp->depthPass() > 0) {
    bmp->setTempTransform(global_transform_);
    depth_mode_bitmaps_.push_back(const_cast<BitmapShape*>(bmp));
    element_cnt_[2]++;
    return;
  }
  QImage transformed_image =
      bmp->sourceImage()
          .transformed(transform, bmp->gradient() ? Qt::SmoothTransformation
                                                  : Qt::FastTransformation)
          .convertToFormat(QImage::Format_ARGB32);
  BitmapHandlerType type;
  if (bmp->gradient()) {
    type = BitmapHandlerType::GradientMode;
    element_cnt_[1]++;
  } else {
    type = BitmapHandlerType::NormalMode;
    element_cnt_[0]++;
  }
  layer_painter_ = std::make_unique<QPainter>(&layer_bitmaps_[type]);
  if (bmp->gradient()) { // gradient mode
    layer_painter_->drawImage(new_dirty_area.topLeft(), transformed_image);
  } else { // binarize mode
    layer_painter_->drawImage(new_dirty_area.topLeft(), imageBinarize(&transformed_image, bmp->thrsh_brightness()));
  }
  layer_painter_->end();
  bitmap_dirty_areas_[type] = bitmap_dirty_areas_[type].united(new_dirty_area);
  QRectF boundary_mm = resolution_scale_transform_.mapRect(machine_work_area_mm_);

  // Boundary check
  if (exceed_boundary_ == false && 
      (bitmap_dirty_areas_[type].top() < boundary_mm.top() * canvas_mm_ratio_ || 
      bitmap_dirty_areas_[type].bottom() > boundary_mm.bottom() * canvas_mm_ratio_ ||
      bitmap_dirty_areas_[type].left() < boundary_mm.left() * canvas_mm_ratio_ || 
      bitmap_dirty_areas_[type].right() > boundary_mm.right() * canvas_mm_ratio_)) {
    exceed_boundary_ = true;
  }
}

void ToolpathExporter::convertPath(const PathShape *path) {
  // qInfo() << "Convert Path" << path;
  // transformed_path: Express path in unit of dots (depends on document resolution settings)
  QPainterPath transformed_path = (path->transform() * global_transform_).map(path->path());
  QRectF boundary_mm = resolution_scale_transform_.mapRect(machine_work_area_mm_);

  // Boundary check
  if (exceed_boundary_ == false && 
      (transformed_path.boundingRect().top() < boundary_mm.top() * canvas_mm_ratio_ || 
      transformed_path.boundingRect().bottom() > boundary_mm.bottom() * canvas_mm_ratio_ ||
      transformed_path.boundingRect().left() < boundary_mm.left() * canvas_mm_ratio_ || 
      transformed_path.boundingRect().right() > boundary_mm.right() * canvas_mm_ratio_)) {
    exceed_boundary_ = true;
  }

  if (path->isStlPlaceholder()) {
    const StlPlacement &placement = path->stlPlacement();
    // Framing / red light only needs the footprint, engraving needs the real 3D geometry.
    if (!is_contour_) {
      bool has_geometry = false;
      if (placement.geometry_kind == StlPlacement::GeometryKind::PointCloud) {
        has_geometry =
            point_cloud_objects_ != nullptr && point_cloud_objects_->contains(placement.id);
      } else if (placement.geometry_kind == StlPlacement::GeometryKind::Mesh) {
        has_geometry = stl_objects_ != nullptr && stl_objects_->contains(placement.id);
      }
      if (!has_geometry) {
        // Discard, but say so: otherwise the user just gets an object that was never engraved.
        qWarning() << "[Export] 3D placeholder" << placement.id
                   << "has no matching geometry payload, the object is skipped";
        return;
      }
      // The kind has to be resolved HERE: the object's own layer is current_layer_ right now, and
      // that is what says whether it is filled. By output time it may be the paired layer's turn.
      layer_stl_placements_.append(StlPlacementJob{placement, stlEngraveKind(placement, path)});
      return;
    }
    // Framing / red light: the footprint of the placeholder is exactly the XY projection we want,
    // so fall through and treat it as an ordinary path.
    qInfo() << "[Export] STL placeholder" << placement.id << "exported as a flat projection";
  }

  // Fill shape
  if ((path->isFilled() && current_layer_->type() == Layer::Type::Mixed) ||
      current_layer_->type() == Layer::Type::Fill ||
      current_layer_->type() == Layer::Type::FillLine) {
    if (is_contour_) {
      polygons_mutex_.lock();
      transformed_path = transformed_path.intersected(canvas_clip_path_);
      QList<QPolygonF> polys = transformed_path.toSubpathPolygons();
      for (QPolygonF& poly : polys) {
        if (!poly.isEmpty() && poly.first() != poly.last()) {
          poly.append(poly.first());
        }
      }
      layer_polygons_.append(polys);
      polygons_mutex_.unlock();
    } else {
      // TODO (Fix overlapping fills inside a single layer)
      // TODO (Consider CacheStack as a primary painter for layers?)
      // layer_painter_->setPen(Qt::NoPen); // Otherwise, the border would occupy at least 1 pixel
      // layer_painter_->setBrush(Qt::black);
      // layer_painter_->drawPath(transformed_path);
      // layer_painter_->setBrush(Qt::NoBrush);
      // bitmap_dirty_area_ = bitmap_dirty_area_.united(transformed_path.boundingRect());
      polygons_mutex_.lock();
      FilledPath filled_path;
      filled_path.isEvenOdd = path->path().fillRule() == Qt::OddEvenFill;
      filled_path.polys = transformed_path.toSubpathPolygons();
      layer_filled_polygons_.append(filled_path);
      polygons_mutex_.unlock();
    }
  }
  // Line shape
  if ((!path->isFilled() && current_layer_->type() == Layer::Type::Mixed) ||
      current_layer_->type() == Layer::Type::Line ||
      current_layer_->type() == Layer::Type::FillLine) {
    polygons_mutex_.lock();
    layer_polygons_.append(transformed_path.toSubpathPolygons());
    polygons_mutex_.unlock();
  }
}

void mergeQList(QList<QPolygonF> &Array, int front, int mid, int end){
  // 利用 QList 的constructor, 
  // 把array[front]~array[mid]放進 LeftSub[]
  // 把array[mid+1]~array[end]放進 RightSub[]
  QList<QPolygonF> LeftSub(Array.begin()+front, Array.begin()+mid+1),
                   RightSub(Array.begin()+mid+1, Array.begin()+end+1);

  LeftSub.insert(LeftSub.end(), QRectF(0,0,std::numeric_limits<double>::max(),1));      // 在LeftSub[]尾端加入值為 Max 的元素
  RightSub.insert(RightSub.end(), QRectF(0,0,std::numeric_limits<double>::max(),1));    // 在RightSub[]尾端加入值為 Max 的元素
 
  int idxLeft = 0, idxRight = 0;
  for (int i = front; i <= end; i++) {
    if (LeftSub[idxLeft].boundingRect().width() <= RightSub[idxRight].boundingRect().width() ) {
      Array[i] = LeftSub[idxLeft];
      idxLeft++;
    }
    else{
      Array[i] = RightSub[idxRight];
      idxRight++;
    }
  }
}

// Performance: O(n log(n))
void mergeSort(QList<QPolygonF> &array, int front, int end) {
                                          // front與end為矩陣範圍
  if (front < end) {                      // 表示目前的矩陣範圍是有效的
    int mid = (front+end)/2;              // mid即是將矩陣對半分的index
    mergeSort(array, front, mid);         // 繼續divide矩陣的前半段subarray
    mergeSort(array, mid+1, end);         // 繼續divide矩陣的後半段subarray
    mergeQList(array, front, mid, end);   // 將兩個subarray做比較, 並合併出排序後的矩陣
  }
}

// Performance: O(n^2)
void nestedSort(QList<QPolygonF> &array) {
  QList<QPolygonF> sort_result;
  for (const auto& polygon: array) {
    int insert_idx = sort_result.size();
    for (int idx = sort_result.size() - 1; idx >= 0; idx--) {
      if (sort_result[idx].boundingRect().contains(polygon.boundingRect())) {
        insert_idx = idx;
      }
    }
    sort_result.insert(insert_idx, polygon);
  }

  array = sort_result;
}

void ToolpathExporter::sortPolygons() {
  if (is_promark_) {
    path_utils_.sortAndPreprocessPolygons(layer_polygons_);
    return;
  }
  switch (sort_rule_)
  {
  case MergeSort:
    mergeSort(layer_polygons_, 0, layer_polygons_.size()-1);
    break;
  case NestedSort:
    nestedSort(layer_polygons_);
    break;
  case NoSort:
  default:
    break;
  }
}

/**
 * @brief Export the layer_polygons_ and layer_bitmap_ converted from objects on canvas 
 *        to generator
 * 
 */
void ToolpathExporter::outputLayerGcode(double progress_start, double progress_span) {
  const bool has_stl = !stl_output_deferred_ && !layer_stl_placements_.isEmpty();
  // STL preparation/slicing can dominate conversion time, so reserve a visible half of this
  // layer's progress for it. Layers without STL retain the existing full-range behaviour.
  const double two_d_span = progress_span * (has_stl ? 0.5 : 1.0);
  progress_increment_scale_ = two_d_span;
  const auto report2d = [&](double value) {
    onProgressChanged(progress_start + value * two_d_span, true);
  };
  const auto stopIfCancelled = [&]() {
    if (!cancelled_) return false;
    progress_increment_scale_ = 1.0;
    return true;
  };

  element_cnt_[3] = layer_filled_polygons_.size();
  element_cnt_[4] = layer_polygons_.size();
  total_element_cnt_ = 0;
  for (int i = 0; i < 5; i++) {
    total_element_cnt_ += element_cnt_[i];
  }
  // Guard the divisions below.
  if (total_element_cnt_ <= 0) {
    total_element_cnt_ = 1;
  }
  report2d(0.05);

  outputLayerBitmapGcode(BitmapHandlerType::NormalMode);
  if (stopIfCancelled()) return;
  report2d(0.05 + 0.95 * element_cnt_[0] / total_element_cnt_);

  outputLayerBitmapGcode(BitmapHandlerType::GradientMode);
  if (stopIfCancelled()) return;
  report2d(0.05 + 0.95 * (element_cnt_[0] + element_cnt_[1]) / total_element_cnt_);

  int temp_cnt = element_cnt_[0];
  element_cnt_[0] = 0; // avoid counting progress twice in rasterBitmap
  rasterBitmapDepthMode(ScanDirectionMode::kBidirectionMode, 0);
  element_cnt_[0] = temp_cnt;
  if (stopIfCancelled()) return;
  report2d(0.05 + 0.95 * (element_cnt_[0] + element_cnt_[1] + element_cnt_[2]) /
                       total_element_cnt_);

  outputLayerFillGcode();
  if (stopIfCancelled()) return;
  report2d(0.05 + 0.95 * (total_element_cnt_ - element_cnt_[4]) / total_element_cnt_);

  outputLayerPathGcode();
  if (stopIfCancelled()) return;
  report2d(1.0);

  if (has_stl) {
    // Nested fill/path emitters still call onProgressChanged() to pump queued cancel signals, but
    // STL reports its own absolute preparation/slicing progress.
    progress_increment_scale_ = 0.0;
    outputLayerStlGcode(progress_start + two_d_span, progress_span - two_d_span);
  }
  progress_increment_scale_ = 1.0;
  if (cancelled_) return;
  onProgressChanged(progress_start + progress_span, true);
}

/**
 * @brief Plan every STL object of the current canvas layer and engrave strictly bottom to top.
 *
 * Line objects share a merged slice ladder; transformed blue-noise surface/shell points join that
 * ladder as ordered point events. All output lands in the same machine-Z buckets, so the axis is
 * strictly increasing and already engraved crack points never sit in front of later work.
 *
 * "Canvas layer", not "exporter layer": a canvas layer holding both filled and unfilled objects
 * reaches the exporter as the pair "x" / "x-filled", and convertLayer() defers the first half so
 * that both halves end up in this one ladder.
 *
 * Inside one Z step the objects are dispatched by kind, always in the same order:
 *
 *     z0: line+fill, line, dot+fill, dot | z1: line+fill, line, dot+fill, dot | ...
 *
 * Each kind needs a different gcode path, so they cannot be emitted in one pass, but they all
 * belong to the same Z and must not cost a second Z travel.
 *
 * Slicing stays in real model Z. Basic refraction maps the finished geometry into a sliding window
 * of 0.0001 mm machine-Z buckets. A bucket is flushed only when later geometry cannot add to it.
 */
void ToolpathExporter::outputLayerStlGcode(double progress_start, double progress_span) {
  if (layer_stl_placements_.isEmpty()) return;
  constexpr double kPreparationFraction = 0.30;
  constexpr double kPlanningFraction = 0.68;
  const auto reportProgress = [&](double value) {
    onProgressChanged(progress_start + progress_span * std::clamp(value, 0.0, 1.0), true);
    return !cancelled_;
  };
  if (!reportProgress(0.0)) return;

  struct StlJob {
    stl::Slicer slicer;
    StlEngraveKind kind;
    /** Already transformed 3D samples for kDot/kDotFill, sorted by real model Z. */
    std::vector<stl::SurfacePoint> dot_points;
    size_t next_dot_point = 0;
    int next_layer_index = 0;
  };
  std::vector<StlJob> jobs;
  jobs.reserve(layer_stl_placements_.size());
  // One entry per (object, slice position); sorting these is what merges the ladders.
  struct PlaneRef {
    double z;
    int job;
  };
  std::vector<PlaneRef> ladder;
  double finest_layer_height = 0;
  double finest_point_spacing = 0;

  qInfo() << "[Export] STL layer:" << refraction_.describe();
  qInfo() << "[Export] STL material model-Z range" << material_min_z_mm_ << "to"
          << material_max_z_mm_ << "mm";

  const auto modelZInMaterial = [&](double model_z_canvas) {
    const double model_z_mm = model_z_canvas / canvas_mm_ratio_;
    constexpr double kBoundsEpsilonMm = 1e-9;
    return model_z_mm >= material_min_z_mm_ - kBoundsEpsilonMm &&
           model_z_mm <= material_max_z_mm_ + kBoundsEpsilonMm;
  };
  const auto discardPointsOutsideMaterial = [&](std::vector<stl::SurfacePoint> *points,
                                                 const QString &id) {
    const size_t before = points->size();
    points->erase(std::remove_if(points->begin(), points->end(), [&](const auto &point) {
                    return !modelZInMaterial(point.position.z);
                  }),
                  points->end());
    const size_t discarded = before - points->size();
    if (discarded > 0) {
      qInfo() << "[Export] STL object" << id << "discarded" << discarded
              << "points outside the material Z range";
    }
  };

  const int placement_count = layer_stl_placements_.size();
  for (int placement_index = 0; placement_index < placement_count; ++placement_index) {
    if (cancelled_) return;
    const StlPlacementJob &entry = layer_stl_placements_[placement_index];
    const StlPlacement &placement = entry.placement;
    const auto reportPreparation = [&](double object_fraction) {
      return reportProgress(kPreparationFraction *
                            (placement_index + std::clamp(object_fraction, 0.0, 1.0)) /
                            placement_count);
    };
    const double layer_height_mm =
        placement.layer_height_mm > 0 ? placement.layer_height_mm : kDefaultStlLayerHeightMm;
    const double point_spacing_mm =
        placement.point_spacing_mm > 0 ? placement.point_spacing_mm : kDefaultStlPointSpacingMm;
    stl::SliceParams params;
    // Slicing planes are real model heights. Refraction never changes the mesh or layer count.
    params.layer_height = layer_height_mm * canvas_mm_ratio_;

    StlJob job;
    // Resolved at collection time, current_layer_ may be the paired layer by now.
    job.kind = entry.kind;
    QString error;
    const int job_index = static_cast<int>(jobs.size());
    if (placement.geometry_kind == StlPlacement::GeometryKind::Photo ||
        placement.geometry_kind == StlPlacement::GeometryKind::PointCloud) {
      job.kind = StlEngraveKind::kDot;
      if (placement.geometry_kind == StlPlacement::GeometryKind::Photo) {
        if (entry.photo == nullptr || entry.photo->sourceImage().isNull() ||
            !(placement.photo_width_mm > 0) || !(placement.photo_height_mm > 0)) {
          qWarning() << "[Export] Photo object" << placement.id
                     << "has no usable image or local dimensions";
          continue;
        }

        const double transformed_width_mm =
            placement.matrix
                .mapVector(QVector3D(static_cast<float>(placement.photo_width_mm), 0, 0))
                .length() /
            canvas_mm_ratio_;
        const double transformed_height_mm =
            placement.matrix
                .mapVector(QVector3D(0, static_cast<float>(placement.photo_height_mm), 0))
                .length() /
            canvas_mm_ratio_;
        const double columns_value = std::ceil(transformed_width_mm / point_spacing_mm);
        const double rows_value = std::ceil(transformed_height_mm / point_spacing_mm);
        if (!std::isfinite(columns_value) || !std::isfinite(rows_value) || columns_value < 1 ||
            rows_value < 1 || columns_value * rows_value > kMaxPhotoSampleCount) {
          qWarning() << "[Export] Photo object" << placement.id
                     << "has an invalid or excessive sampling grid" << columns_value << "x"
                     << rows_value;
          continue;
        }

        const int columns = static_cast<int>(columns_value);
        const int rows = static_cast<int>(rows_value);
        // Match the ordinary backend bitmap path: composite transparency onto white, resample to
        // the physical dot grid, then let Qt perform diffuse dithering. The SVG contains grayscale
        // source pixels; no frontend-generated dot map is needed.
        QImage sampled_image(QSize(columns, rows), QImage::Format_ARGB32);
        sampled_image.fill(Qt::white);
        QPainter sampled_painter(&sampled_image);
        sampled_painter.setRenderHint(QPainter::SmoothPixmapTransform, entry.photo->gradient());
        sampled_painter.drawImage(sampled_image.rect(), entry.photo->sourceImage());
        sampled_painter.end();

        QImage binary_image;
        if (entry.photo->gradient()) {
          binary_image =
              sampled_image
                  .convertToFormat(QImage::Format_Mono, Qt::MonoOnly | Qt::DiffuseDither)
                  .convertToFormat(QImage::Format_Grayscale8);
        } else {
          QImage grayscale_image =
              sampled_image.convertToFormat(QImage::Format_Grayscale8)
                  .convertToFormat(QImage::Format_ARGB32);
          binary_image = imageBinarize(&grayscale_image, entry.photo->thrsh_brightness());
        }

        job.dot_points.reserve(static_cast<size_t>(columns) * static_cast<size_t>(rows));
        for (int row = 0; row < rows; ++row) {
          if ((row & 31) == 0 && !reportPreparation(0.1 + 0.7 * row / rows)) return;
          const double y_ratio = (row + 0.5) / rows;
          const uchar *pixels = binary_image.constScanLine(row);
          for (int column = 0; column < columns; ++column) {
            if (pixels[column] != 0) continue;

            const double x_ratio = (column + 0.5) / columns;
            const QVector3D transformed = placement.matrix.map(QVector3D(
                static_cast<float>((x_ratio - 0.5) * placement.photo_width_mm),
                static_cast<float>((0.5 - y_ratio) * placement.photo_height_mm), 0));
            job.dot_points.push_back(stl::SurfacePoint{
                stl::Vec3{transformed.x(), transformed.y(), transformed.z()}, 0});
          }
        }
      } else {
        if (point_cloud_objects_ == nullptr) continue;
        auto cloud_it = point_cloud_objects_->constFind(placement.id);
        if (cloud_it == point_cloud_objects_->constEnd()) continue;

        job.dot_points.reserve(cloud_it->points.size());
        for (size_t point_index = 0; point_index < cloud_it->points.size(); ++point_index) {
          if ((point_index & 4095) == 0 &&
              !reportPreparation(0.1 + 0.7 * static_cast<double>(point_index) /
                                           std::max<size_t>(1, cloud_it->points.size()))) {
            return;
          }
          const stl::Vec3 &point = cloud_it->points[point_index];
          const QVector3D transformed = placement.matrix.map(
              QVector3D(static_cast<float>(point.x), static_cast<float>(point.y),
                        static_cast<float>(point.z)));
          job.dot_points.push_back(stl::SurfacePoint{
              stl::Vec3{transformed.x(), transformed.y(), transformed.z()}, 0});
        }
      }
      discardPointsOutsideMaterial(&job.dot_points, placement.id);
      if (job.dot_points.empty()) {
        qWarning() << "[Export] Direct-point object" << placement.id << "has no engravable points";
        continue;
      }
      std::sort(job.dot_points.begin(), job.dot_points.end(), [](const auto &a, const auto &b) {
        if (a.position.z != b.position.z) return a.position.z < b.position.z;
        if (a.position.y != b.position.y) return a.position.y < b.position.y;
        return a.position.x < b.position.x;
      });
      const double spacing = point_spacing_mm * canvas_mm_ratio_;
      if (finest_point_spacing <= 0 || spacing < finest_point_spacing) {
        finest_point_spacing = spacing;
      }
      qInfo() << "[Export] Direct-point object" << placement.id << "prepared,"
              << job.dot_points.size() << "direct points";
      jobs.push_back(std::move(job));
      if (!reportPreparation(1.0)) return;
      continue;
    }

    if (stl_objects_ == nullptr) continue;
    auto mesh_it = stl_objects_->constFind(placement.id);
    if (mesh_it == stl_objects_->constEnd()) continue;

    if (job.kind == StlEngraveKind::kDot || job.kind == StlEngraveKind::kDotFill) {
      stl::BlueNoiseParams blue_noise;
      // The placement matrix ends in canvas units (10/mm), so both Poisson distance and shell
      // offset are evaluated after the 4x4 transform in that same real-geometry space.
      blue_noise.spacing = point_spacing_mm * canvas_mm_ratio_;
      // Preserve the two existing controls: point spacing is density on a shell, layer height is
      // now the distance between inward shells for kDotFill.
      blue_noise.shell_spacing = layer_height_mm * canvas_mm_ratio_;
      stl::PointCloudResult cloud =
          job.kind == StlEngraveKind::kDot
              ? stl::sampleSurfaceBlueNoise(mesh_it.value(), placement.matrix, blue_noise,
                                            reportPreparation)
              : stl::sampleInwardShellsBlueNoise(mesh_it.value(), placement.matrix, blue_noise,
                                                 reportPreparation);
      if (cancelled_) return;
      if (!cloud.ok) {
        qWarning() << "[Export] Failed to sample STL object" << placement.id << cloud.error;
        continue;
      }
      if (cloud.shell_limit_reached) {
        qWarning() << "[Export] STL inward shells reached safety limit" << blue_noise.max_shell_count
                   << "for object" << placement.id;
      }
      job.dot_points = std::move(cloud.points);
      discardPointsOutsideMaterial(&job.dot_points, placement.id);
      if (job.dot_points.empty()) {
        qWarning() << "[Export] STL object" << placement.id
                   << "has no points inside the material Z range";
        continue;
      }
      std::sort(job.dot_points.begin(), job.dot_points.end(), [](const auto &a, const auto &b) {
        if (a.position.z != b.position.z) return a.position.z < b.position.z;
        if (a.shell_index != b.shell_index) return a.shell_index < b.shell_index;
        if (a.position.y != b.position.y) return a.position.y < b.position.y;
        return a.position.x < b.position.x;
      });
      if (finest_point_spacing <= 0 || blue_noise.spacing < finest_point_spacing) {
        finest_point_spacing = blue_noise.spacing;
      }
      qInfo() << "[Export] STL object" << placement.id << "sampled," << job.dot_points.size()
              << "blue-noise points," << cloud.shell_count << "shells, kind"
              << static_cast<int>(job.kind) << "seed" << QString::number(stl::kBlueNoiseSeed, 16);
    } else {
      if (!job.slicer.prepare(mesh_it.value(), placement.matrix, params, &error,
                              [&](double value) { return reportPreparation(value * 0.8); })) {
        if (cancelled_) return;
        qWarning() << "[Export] Failed to prepare STL object" << placement.id << error;
        continue;
      }
      // Adaptive slicing is a per-object opt-in. Filled lines retain their fixed-plane path.
      const bool adaptive = job.kind == StlEngraveKind::kLine &&
                            placement.min_layer_height_mm > 0.0;
      const QVector<double> planes =
          adaptive ? job.slicer.adaptivePlanes(
                         placement.min_layer_height_mm * canvas_mm_ratio_,
                         [&](double value) { return reportPreparation(0.8 + value * 0.2); })
                   : job.slicer.planes();
      if (cancelled_) return;
      int accepted_plane_count = 0;
      for (double z : planes) {
        if (!modelZInMaterial(z)) continue;
        ladder.push_back(PlaneRef{z, job_index});
        ++accepted_plane_count;
      }
      if (finest_layer_height <= 0 || params.layer_height < finest_layer_height) {
        finest_layer_height = params.layer_height;
      }
      qInfo() << "[Export] STL object" << placement.id << "prepared," << accepted_plane_count
              << (adaptive ? "adaptive layers," : "fixed layers,")
              << "kind" << static_cast<int>(job.kind);
    }
    jobs.push_back(std::move(job));
    if (!reportPreparation(1.0)) return;
  }
  if (jobs.empty()) {
    reportProgress(1.0);
    return;
  }

  std::sort(ladder.begin(), ladder.end(),
            [](const PlaneRef &a, const PlaneRef &b) { return a.z < b.z; });
  if (!reportProgress(kPreparationFraction)) return;
  // Objects with different layer heights will not land on exactly the same plane, so slices closer
  // than a thousandth of the finest layer are treated as one Z step.
  const double group_eps = finest_layer_height > 0 ? finest_layer_height * 1e-3 : 1e-9;
  // Continuous surface samples would otherwise make the event loop wake once per point. Batching a
  // quarter spacing at a time does not merge machine-Z buckets; it only amortises planning work.
  const double point_batch_span = finest_point_spacing > 0 ? finest_point_spacing * 0.25 : 0.0;

  stl_focus_z_mm_ = 0;
  // STL dot mode must never inherit generic path interpolation, which would create extra dots.
  stl_output_active_ = true;
  bool dotting_enabled = false;

  struct DotPoint {
    QPointF target_mm;
  };
  struct MachineBucket {
    QList<stl::Layer> lines[2];
    QVector<DotPoint> dot_cloud[2];         // blue-noise dot+fill, dot
  };
  std::map<qint64, MachineBucket> buckets;
  constexpr double kMachineZBucketMm = 0.0001;
  double total_work = static_cast<double>(ladder.size());
  for (const StlJob &job : jobs) total_work += static_cast<double>(job.dot_points.size());
  total_work = std::max(1.0, total_work);
  double processed_work = 0.0;
  const auto reportPlanning = [&](double in_flight = 0.0) {
    return reportProgress(kPreparationFraction +
                          kPlanningFraction *
                              std::min(1.0, (processed_work + in_flight) / total_work));
  };
  const auto bucketKey = [&](double z_mm) {
    return static_cast<qint64>(std::llround(z_mm / kMachineZBucketMm));
  };
  const auto bucketZ = [&](qint64 key) { return key * kMachineZBucketMm; };

  auto addBlueNoisePointToBucket = [&](const stl::SurfacePoint &sample, int dot_kind) {
    const QPointF target_dots = global_transform_.map(
        QPointF(sample.position.x, sample.position.y));
    const QPointF target_mm = target_dots / dpmm_;
    const double model_z_mm = sample.position.z / canvas_mm_ratio_;
    const qint64 key = bucketKey(refraction_.machineZ(model_z_mm));
    buckets[key].dot_cloud[dot_kind].append(DotPoint{target_mm});
  };

  auto setDotting = [&](bool enabled) {
    if (enabled == dotting_enabled) return;
    gen_->setDottingTime(enabled ? current_layer_->dottingTime() : 0);
    if (is_path_preview_) gen_->setDottingMode(enabled);
    dotting_enabled = enabled;
  };
  auto clearGeometry = [&]() {
    QMutexLocker lock(&polygons_mutex_);
    layer_polygons_.clear();
    layer_filled_polygons_.clear();
  };
  auto flushBucket = [&](qint64 key, MachineBucket &bucket) {
    if (!reportPlanning()) return false;
    const double target_z_mm = bucketZ(key);
    if (target_z_mm < 0.0) {
      qWarning() << "[Export] skipping unreachable STL machine Z" << target_z_mm;
      return true;
    }
    moveStlFocusZ(target_z_mm);
    setDotting(false);
    stl_dot_mode_ = false;
    if (!bucket.lines[0].isEmpty()) {
      clearGeometry();
      outputStlLineFillGcode(bucket.lines[0]);
      if (cancelled_) return false;
    }
    if (!bucket.lines[1].isEmpty()) {
      clearGeometry();
      outputStlLineGcode(bucket.lines[1]);
      if (cancelled_) return false;
    }
    for (int dot_kind = 0; dot_kind < 2; ++dot_kind) {
      QVector<DotPoint> &cloud = bucket.dot_cloud[dot_kind];
      if (cloud.isEmpty()) continue;
      setDotting(true);
      stl_dot_mode_ = true;
      clearGeometry();
      QPolygonF points;
      points.reserve(cloud.size());
      for (const DotPoint &point : cloud) points << point.target_mm;
      const QVector<int> cloud_order = stl::nearestPointOrder(
          points, current_pos_mm_, [&](double) { return reportPlanning(); });
      if (cancelled_) return false;
      QPolygonF cloud_path;
      cloud_path.reserve(cloud.size());
      for (int index : cloud_order) cloud_path << cloud[index].target_mm * dpmm_;
      {
        QMutexLocker lock(&polygons_mutex_);
        layer_polygons_.append(std::move(cloud_path));
      }
      outputLayerPathGcode();
      if (cancelled_) return false;
    }
    return true;
  };

  QList<stl::Layer> by_kind[2];
  struct PointCursor {
    double z = 0.0;
    int job = -1;
  };
  struct PointCursorLater {
    bool operator()(const PointCursor &a, const PointCursor &b) const {
      return a.z > b.z || (a.z == b.z && a.job > b.job);
    }
  };
  std::priority_queue<PointCursor, std::vector<PointCursor>, PointCursorLater> point_queue;
  for (int job_index = 0; job_index < static_cast<int>(jobs.size()); ++job_index) {
    if (!jobs[static_cast<size_t>(job_index)].dot_points.empty()) {
      point_queue.push(PointCursor{
          jobs[static_cast<size_t>(job_index)].dot_points.front().position.z, job_index});
    }
  }

  size_t i = 0;
  while (i < ladder.size() || !point_queue.empty()) {
    if (this->cancelled_) break;
    const double next_line_z = i < ladder.size() ? ladder[i].z
                                                  : std::numeric_limits<double>::infinity();
    const double next_point_z = !point_queue.empty() ? point_queue.top().z
                                                      : std::numeric_limits<double>::infinity();
    const double step_z = std::min(next_line_z, next_point_z);

    for (QList<stl::Layer> &bucket : by_kind) bucket.clear();
    while (i < ladder.size() && ladder[i].z - step_z <= group_eps) {
      StlJob &job = jobs[static_cast<size_t>(ladder[i].job)];
      stl::Layer sliced = job.slicer.sliceAt(
          ladder[i].z, [&](double value) { return reportPlanning(value); });
      if (cancelled_) break;
      sliced.index = job.next_layer_index++;
      if (!sliced.contours.isEmpty()) {
        by_kind[static_cast<int>(job.kind)].append(std::move(sliced));
      }
      ++i;
      processed_work += 1.0;
    }
    if (cancelled_) break;
    while (!point_queue.empty() && point_queue.top().z - step_z <= point_batch_span + 1e-12) {
      const PointCursor cursor = point_queue.top();
      point_queue.pop();
      StlJob &job = jobs[static_cast<size_t>(cursor.job)];
      const stl::SurfacePoint &sample = job.dot_points[job.next_dot_point];
      const int dot_kind = job.kind == StlEngraveKind::kDotFill ? 0 : 1;
      addBlueNoisePointToBucket(sample, dot_kind);
      ++job.next_dot_point;
      processed_work += 1.0;
      if ((job.next_dot_point & 1023) == 0 && !reportPlanning()) break;
      if (job.next_dot_point < job.dot_points.size()) {
        point_queue.push(PointCursor{job.dot_points[job.next_dot_point].position.z, cursor.job});
      }
    }
    if (cancelled_) break;
    for (int k = 0; k < 2; ++k) {
      if (this->cancelled_) break;
      const QList<stl::Layer> &slices = by_kind[k];
      if (slices.isEmpty()) continue;
      for (const stl::Layer &slice : slices) {
        const double model_z_mm = slice.z_geometry / canvas_mm_ratio_;
        const qint64 key = bucketKey(refraction_.machineZ(model_z_mm));
        buckets[key].lines[k].append(slice);
      }
    }
    if (this->cancelled_) break;
    const double following_line_z = i < ladder.size() ? ladder[i].z
                                                       : std::numeric_limits<double>::infinity();
    const double following_point_z = !point_queue.empty() ? point_queue.top().z
                                                           : std::numeric_limits<double>::infinity();
    const double next_geometry_z = std::min(following_line_z, following_point_z);
    const double next_model_z_mm = std::isfinite(next_geometry_z)
                                       ? next_geometry_z / canvas_mm_ratio_
                                       : std::numeric_limits<double>::infinity();
    const qint64 flush_before = std::isfinite(next_model_z_mm)
                                    ? bucketKey(refraction_.machineZ(next_model_z_mm))
                                    : std::numeric_limits<qint64>::max();
    while (!buckets.empty() && buckets.begin()->first < flush_before) {
      if (!flushBucket(buckets.begin()->first, buckets.begin()->second)) break;
      buckets.erase(buckets.begin());
    }
    if (!reportPlanning()) break;
  }

  for (auto &entry : buckets) {
    if (this->cancelled_) break;
    if (!flushBucket(entry.first, entry.second)) break;
  }

  stl_output_active_ = false;
  stl_dot_mode_ = false;
  // Cancellation adds no STL-specific cleanup commands here. Only a normally completed STL pass
  // disables dotting, returns to the focus origin and reports completion.
  if (!cancelled_) {
    if (dotting_enabled) gen_->setDottingTime(0);
    moveStlFocusZ(0);
    reportProgress(1.0);
  }
}

void ToolpathExporter::moveStlFocusZ(double focus_z_mm) {
  // Quantise to the forced 0.0001mm STL bucket. The moves are relative, so a target the
  // generator rounds away would still count here and the two positions would slowly drift apart --
  // over many layers that adds up.
  const double target_z_mm = std::round(focus_z_mm * 10000.0) / 10000.0;
  if (target_z_mm == stl_focus_z_mm_) return;
  // moveZ is relative for Promark. A positive delta raises the head, matching focus_z_mm.
  gen_->moveZ((target_z_mm - stl_focus_z_mm_));
  stl_focus_z_mm_ = target_z_mm;
}

/**
 * Emit one STL polygon. Z is handled by the shared layer ladder before this function is called.
 */
void ToolpathExporter::emitStlPolygon(const QPolygonF &poly_dots, double speed, double power) {
  if (poly_dots.isEmpty() || cancelled_) return;
  if (stl_dot_mode_) {
    // Every point is a dot; inserting interpolation points would change the requested density.
    moveTo(poly_dots.first() / dpmm_, travel_speed_, 0, 0);
    for (int index = 0; index < poly_dots.size(); ++index) {
      if ((index & 1023) == 0) {
        onProgressChanged(current_progress_, true);
        if (cancelled_) return;
      }
      moveTo(poly_dots[index] / dpmm_, speed, power, 0);
    }
    return;
  }

  moveTo(poly_dots.first() / dpmm_, travel_speed_, 0, 0);
  for (int index = 0; index < poly_dots.size(); ++index) {
    if ((index & 1023) == 0) {
      onProgressChanged(current_progress_, true);
      if (cancelled_) return;
    }
    moveTo(poly_dots[index] / dpmm_, speed, power, 0);
  }
  moveTo(poly_dots.last() / dpmm_, travel_speed_, 0, 0);
}

/** Emit one scan-line segment of an STL fill. */
void ToolpathExporter::emitStlFillSegment(const QPointF &start_dots, const QPointF &end_dots) {
  const QPointF start_mm = start_dots / dpmm_;
  const QPointF end_mm = end_dots / dpmm_;
  moveTo(start_mm, current_layer_->speed(), 0, 0);
  moveTo(end_mm, current_layer_->speed(), current_layer_->power(), 0);
}

StlEngraveKind ToolpathExporter::stlEngraveKind(const StlPlacement &placement,
                                                const PathShape *path) const {
  if (placement.geometry_kind == StlPlacement::GeometryKind::Photo ||
      placement.geometry_kind == StlPlacement::GeometryKind::PointCloud) {
    return StlEngraveKind::kDot;
  }

  const bool filled = (path->isFilled() && current_layer_->type() == Layer::Type::Mixed) ||
                      current_layer_->type() == Layer::Type::Fill ||
                      current_layer_->type() == Layer::Type::FillLine;
  if (placement.mode == StlPlacement::Mode::Dot) {
    return filled ? StlEngraveKind::kDotFill : StlEngraveKind::kDot;
  }
  return filled ? StlEngraveKind::kLineFill : StlEngraveKind::kLine;
}

/**
 * Map the contours of one slice into document dots.
 * The 2D transform of the placeholder shape is deliberately NOT applied: the placement matrix is
 * the authoritative transform for the mesh, the rect only mirrors its XY projection.
 */
static QList<QPolygonF> mapStlContours(const QTransform &transform, const stl::Layer &layer) {
  QList<QPolygonF> polys;
  polys.reserve(layer.contours.size());
  for (const stl::Contour &contour : layer.contours) {
    QPolygonF poly = transform.map(contour.polygon);
    if (poly.isEmpty()) continue;
    polys.append(std::move(poly));
  }
  return polys;
}

/** Line + fill: the contours bound the area, outputLayerFillGcode scans it. */
void ToolpathExporter::outputStlLineFillGcode(const QList<stl::Layer> &slices) {
  {
    QMutexLocker lock(&polygons_mutex_);
    for (const stl::Layer &slice : slices) {
      FilledPath filled_path;
      // The contours of a slice are properly nested and never overlap, so even odd fill is both
      // correct and independent of the winding.
      filled_path.isEvenOdd = true;
      // One FilledPath per object: an object has to be filled together with its own holes, and two
      // objects overlapping in XY must not punch holes into each other.
      filled_path.polys = mapStlContours(global_transform_, slice);
      if (!filled_path.polys.isEmpty()) layer_filled_polygons_.append(filled_path);
    }
  }
  // The layer's own scan settings apply, only the per Z step logging is silenced.
  outputLayerFillGcode(true);
}

/** Line, no fill: the contours are the toolpath. */
void ToolpathExporter::outputStlLineGcode(const QList<stl::Layer> &slices) {
  {
    QMutexLocker lock(&polygons_mutex_);
    for (const stl::Layer &slice : slices) {
      layer_polygons_.append(mapStlContours(global_transform_, slice));
    }
  }
  outputLayerPathGcode();
}

/**
 * @brief Export layer_filled_polygons_ for non-filled geometry
 * @param quiet suppresses per-call logging for STL layers with many Z planes
 */
void ToolpathExporter::outputLayerFillGcode(bool quiet) {
  struct Path {
    QLineF path;
    bool isClockwise;
  };
  struct PathGroup {
    QList<Path> paths;
    bool isEvenOdd;
  };
  struct Intersection {
    QPointF point;
    bool isClockwise;
  };

  QRectF bounds;
  QMutexLocker polygons_lock(&polygons_mutex_);
  for (auto &paths : layer_filled_polygons_) {
    if (paths.polys.empty()) continue;
    for (const auto& poly : paths.polys) {
      if (poly.empty()) continue;
      bounds = bounds.united(poly.boundingRect());
    }
  }
  const bool verbose = !quiet;
  if (verbose) {
    qInfo() << "Fill Path Bounds: " << bounds;
    qInfo() << "DPMM: " << dpmm_;
  }
  // If DPI = 254, DPMM = 10, CANVAS_MM_RATIO = 10
  double fill_interval = current_layer_->fillInterval() * dpmm_;
  double fill_angle = current_layer_->fillAngle();
  bool fill_bidirectional = current_layer_->fillBidirectional();
  int hatch_count = current_layer_->fillHatch() ? 2 : 1;
  if (fill_interval <= 0) fill_interval = 1;
  if (hatch_count < 1) hatch_count = 1;
  // UV heat mitigation. A UV spot fed continuously inside a small area accumulates heat and burns
  // deeper than intended, so the fill may be spread out in two independent ways: engrave the scan
  // lines interleaved instead of adjacent, and pause between them.
  const int stagger_groups = std::max(1, current_layer_->fillStagger());
  // The U command is a FLUX extension only the Promark controller understands.
  const int fill_dwell_us = is_promark_ ? std::max(0, current_layer_->fillDwellTime()) : 0;
  const bool fill_dwell_adaptive = current_layer_->fillDwellAdaptive();

  // Calculate diagonal length to ensure coverage
  double diagonal = qSqrt(bounds.width() * bounds.width() +
                          bounds.height() * bounds.height()) * 1.1;
  if (verbose) qInfo() << "Diagonal: " << diagonal / dpmm_;
  if (diagonal == 0) {
    return;
  }

  // Report progress every 1 percent
  float progress_unit = 0.0095 * element_cnt_[3] / total_element_cnt_;
  int progress_batch = (hatch_count * diagonal * 1.5 / fill_interval) / 100 + 1;
  int cnt = -1;

  for (int hatch = 0; hatch < hatch_count; hatch++) {
    if (this->cancelled_) return;
    // Convert angle to radians
    double angleRad = qDegreesToRadians(fill_angle);

    // Calculate perpendicular direction for scanning
    QPointF direction(qCos(angleRad), qSin(angleRad));
    QPointF perpendicular(-direction.y(), direction.x());

    // Calculate center point
    QPointF center = bounds.center();
    if (verbose) qInfo() << "Center Point: " << center / dpmm_;

    // Calculate start point (offset by half diagonal in perpendicular direction)
    QPointF start = center - (perpendicular * diagonal / 2);
    if (verbose) qInfo() << "Start Point: " << start / dpmm_;

    QList<PathGroup> all_paths;
    for (const auto& paths : layer_filled_polygons_) {
      if (paths.polys.empty())
        continue;
      PathGroup pathGroup;
      for (const auto& poly : paths.polys) {
        if (poly.empty())
          continue;
        for (int i = 0; i < poly.size(); ++i) {
          QPointF curr = poly[i];
          QPointF next = poly[(i + 1) % poly.size()]; // Wrap around to first point
          QLineF pathSegment(curr, next);
          Path pathObj;
          pathObj.path = pathSegment;
          double x1 = direction.x();
          double y1 = direction.y();
          double x2 = next.x() - curr.x();
          double y2 = next.y() - curr.y();
          double dir = x1 * y2 - x2 * y1;
          if (dir == 0) continue;
          pathObj.isClockwise = dir < 0;
          pathGroup.paths.append(pathObj);
        }
      }
      if (pathGroup.paths.size() == 0) continue;
      pathGroup.isEvenOdd = paths.isEvenOdd;
      all_paths.append(pathGroup);
    }

    gen_->turnOnLaser();

    // Scan across the path.
    // A scan line sits at perpendicular coordinate (offset - diagonal / 2) relative to the center,
    // so it can only reach the shape while that stays inside the projection of bounds onto the
    // perpendicular axis. The grid deliberately spans more than the shape, but it is not centered
    // on it: the offsets below first_offset alone are over half of the range for a square at angle
    // 0. Those lines never produced anything, they only cost an intersection pass -- and once the
    // lines are engraved in a staggered order they would also pad the line count and skew every
    // group. The grid itself is unchanged, so the engraved lines stay exactly where they were.
    const double half_extent = (std::fabs(bounds.width() * perpendicular.x()) +
                                std::fabs(bounds.height() * perpendicular.y())) / 2 +
                               fill_interval;
    const double first_offset = diagonal / 2 - half_extent;
    const double last_offset = diagonal / 2 + half_extent;
    QList<double> scan_offsets;
    for (double offset = -diagonal / 2; offset <= diagonal; offset += fill_interval) {
      if (offset < first_offset || offset > last_offset) continue;
      scan_offsets.append(offset);
    }
    // Split the scan lines into stagger_groups contiguous blocks and take one line from each block
    // in turn, so consecutive laser work never stays inside the same narrow band. 40 lines with
    // stagger 4 engrave as 1, 11, 21, 31, 2, 12, 22, 32, ... A stagger of 1 keeps the plain
    // spatial order.
    const int scan_line_count = scan_offsets.size();
    const int stagger_step =
        stagger_groups > 1
            ? std::max(1, (scan_line_count + stagger_groups - 1) / stagger_groups)
            : 1;
    QList<int> scan_order;
    scan_order.reserve(scan_line_count);
    for (int first = 0; first < stagger_step; ++first) {
      for (int index = first; index < scan_line_count; index += stagger_step) {
        scan_order.append(index);
      }
    }
    if (verbose) qInfo() << "Scan lines: " << scan_line_count << " stagger step: " << stagger_step;
    // Keep reporting roughly once per percent now that the empty lines no longer inflate the count
    progress_batch = std::max<int>(1, hatch_count * scan_line_count / 100);

    bool reverse = false;
    for (int order_index = 0; order_index < scan_order.size(); ++order_index) {
      const double offset = scan_offsets[scan_order[order_index]];
      if (this->cancelled_) {
        return;
      }
      if (++cnt % progress_batch == 0) {
        onProgressChanged(progress_unit, false);
      }
      // Calculate line start and end points
      QPointF lineStart = start + perpendicular * offset - direction * diagonal / 2;
      QPointF lineEnd = lineStart + direction * diagonal;
      if (reverse) std::swap(lineStart, lineEnd);
      QLineF scanLine(lineStart, lineEnd);
      std::function<bool(const QPointF& a, const QPointF& b)> isCloser =
          [&lineStart](const QPointF& a, const QPointF& b) {
            return QLineF(lineStart, a).length() <
                   QLineF(lineStart, b).length();
          };
      std::function<bool(const Intersection& a, const Intersection& b)> isCloserIntersection =
          [&lineStart](const Intersection& a, const Intersection& b) {
            return QLineF(lineStart, a.point).length() <
                   QLineF(lineStart, b.point).length();
          };

      QPointF innerStart(lineStart);
      QPointF innerEnd(lineEnd);
      if (path_utils_.clipWorkarea(&innerStart, &innerEnd) == -1) continue;

      // Get intersections with path
      QList<QList<QPointF>> all_intersections;
      QList<QPointF> merged_intersections;
      QList<int> indices(layer_filled_polygons_.size());
      for (const auto& elem : all_paths) {
        QList<Intersection> intersections;
        QList<QPointF> intersection_points;
        for (const auto& path : elem.paths) {
          QPointF intersection;
          if (scanLine.intersects(path.path, &intersection) == QLineF::BoundedIntersection) {
            // Ignore intersections within merged path
            Intersection intersectionObj;
            intersectionObj.point = intersection;
            intersectionObj.isClockwise = path.isClockwise;
            intersections.append(intersectionObj);
          }
        }
        if (intersections.size() == 0) continue;
        // Sort intersections by distance from line start
        std::sort(intersections.begin(), intersections.end(), isCloserIntersection);

        bool is_laser_on = false;
        int sum = 0;
        for (int i = 0; i < intersections.size(); i += 1) {
          Intersection intersection = intersections[i];
          if (intersection.isClockwise) sum += 1;
          else sum -= 1;
          bool should_laser_off = elem.isEvenOdd ? sum % 2 == 0 : sum == 0;
          if (is_laser_on == should_laser_off) {
            // Current state is different from previous state
            is_laser_on = !is_laser_on;
            if (isCloser(intersection.point, innerStart)) {
              intersection_points.append(innerStart);
            } else if (isCloser(innerEnd, intersection.point)) {
              intersection_points.append(innerEnd);
            } else {
              intersection_points.append(intersection.point);
            }
          }
        }
        all_intersections.append(intersection_points);
      }

      // Combine intersections of all polygons
      // Find laser on/off pairs in each loop
      merged_intersections.append(innerStart);
      merged_intersections.append(innerStart);
      QPointF lastOffPoint = innerStart;
      bool hasReachEnd = false;
      while (!hasReachEnd) {
        QPointF currentOnPoint = innerEnd;
        int currentIdx = -1;
        for (int i = 0; !hasReachEnd && i < all_intersections.size(); ++i) {
          for (int j = indices[i]; !hasReachEnd && j < all_intersections[i].size(); j += 2) {
            QPointF start = all_intersections[i][j];
            if (isCloser(start, lastOffPoint) || start == lastOffPoint) {
              // Handle overlapping intersections
              QPointF end = all_intersections[i][j + 1];
              if (isCloser(end, lastOffPoint)) {
                // Skip completed included intersections
                // This will also skip those intersections that end before inner start
                indices[i] += 2;
                continue;
              }
              if (!isCloser(end, innerEnd)) {
                // Clip end point
                end = innerEnd;
                hasReachEnd = true;
              } else {
                indices[i] += 2;
              }
              // Merge overlapping intersections by updating laser off point
              lastOffPoint = end;
              merged_intersections.last() = end;
              continue;
            } else if (isCloser(start, currentOnPoint)) {
              // Find the next intersection without overlapping
              currentOnPoint = start;
              currentIdx = i;
            }
            break;
          }
        }
        if (currentIdx == -1)
          break;

        merged_intersections.append(currentOnPoint);
        lastOffPoint = all_intersections[currentIdx][indices[currentIdx] + 1];
        if (!isCloser(lastOffPoint, innerEnd)) {
          // Clip end point
          lastOffPoint = innerEnd;
          hasReachEnd = true;
        }
        merged_intersections.append(lastOffPoint);
        indices[currentIdx] += 2;
      }

      // Process pairs of intersections
      double marked_length_mm = 0;
      for (int i = 0; i < merged_intersections.size() - 1; i += 2) {
        if (merged_intersections[i] == merged_intersections[i + 1]) {
          // Skip zero length segments
          continue;
        }

        marked_length_mm +=
            QLineF(merged_intersections[i], merged_intersections[i + 1]).length() / dpmm_;

        if (stl_output_active_) {
          emitStlFillSegment(merged_intersections[i], merged_intersections[i + 1]);
          continue;
        }

        // Move to start point with no laser
        moveTo(merged_intersections[i] / dpmm_, current_layer_->speed(), 0, 0);

        // Move to end point
        moveTo(merged_intersections[i + 1] / dpmm_, current_layer_->speed(), current_layer_->power(), 0);
      }

      // Let the material cool down before the next scan line. In adaptive mode the time this line
      // already spent marking counts towards the pause, so a line long enough to have spread the
      // heat by itself waits less, or not at all.
      if (fill_dwell_us > 0 && marked_length_mm > 0) {
        double dwell_us = fill_dwell_us;
        if (fill_dwell_adaptive && current_layer_->speed() > 0) {
          dwell_us -= 1e6 * marked_length_mm / current_layer_->speed();
        }
        if (dwell_us >= 1) gen_->dwell(static_cast<int>(std::lround(dwell_us)));
      }

      if (fill_bidirectional) reverse = !reverse;
    }
    fill_angle += 90;
  }
  gen_->turnOffLaser();
}

/**
 * @brief Export layer_polygons_ for non-filled geometry
 */
void ToolpathExporter::outputLayerPathGcode() {
  double wobble_step = current_layer_->wobbleStep();
  double wobble_diameter = current_layer_->wobbleDiameter();
  if (!is_contour_ && wobble_step > 0 && wobble_diameter > 0) {
    gen_->setWobble(wobble_step, wobble_diameter);
  }

  gen_->turnOnLaser(); // M3
  float layer_speed = is_contour_ ? travel_speed_ : current_layer_->speed();
  float layer_power = is_contour_ ? 0 : current_layer_->power();

  // NOTE: Should convert points from canvas unit to mm
  QMutexLocker polygons_lock(&polygons_mutex_);
  for (auto &poly : layer_polygons_) {
    onProgressChanged(current_progress_, true);
    if (cancelled_) break;
    if (poly.empty()) continue;

    if (stl_output_active_) {
      emitStlPolygon(poly, layer_speed, layer_power);
      continue;
    }

    QPointF next_point_mm;

    moveTo(poly.first() / dpmm_, travel_speed_, 0, 0);
    for (int point_index = 0; point_index < poly.size(); ++point_index) {
      if ((point_index & 1023) == 0) {
        onProgressChanged(current_progress_, true);
        if (cancelled_) break;
      }
      const QPointF &point = poly[point_index];
      next_point_mm = point / dpmm_;
      // Divide a long line into small segments
      if (!is_promark_ && (next_point_mm - current_pos_mm_).manhattanLength() > 5) { // At most 5mm per segment
        int segments = std::max(2.0,
                           std::sqrt(std::pow(next_point_mm.x() - current_pos_mm_.x(), 2) +
                                std::pow(next_point_mm.y() - current_pos_mm_.y(), 2)) / 5 );
        QList<QPointF> interpolate_points;
        for (int i = 1; i <= segments; i++) {
          interpolate_points << (i * next_point_mm + (segments - i) * current_pos_mm_) / float(segments);
        }
        for (const QPointF &interpolate_point : interpolate_points) {
          moveTo(interpolate_point, layer_speed, layer_power, 0);
        }
      } else {
        moveTo(next_point_mm, layer_speed, layer_power, 0);
      }
    }
    if (cancelled_) break;
    moveTo(poly.last() / dpmm_, travel_speed_, 0, 0);

    //gen_->turnOffLaser();
  }
  // gen_->moveTo(gen_->x(), gen_->y(), current_layer_->speed(), 0, 0);
  gen_->turnOffLaser();
  if (wobble_step > 0 && wobble_diameter > 0) {
    gen_->setWobble(0, 0);
  }
}

/**
 * @brief Export layer_bitmap_ for filled geometry and images
 */
void ToolpathExporter::outputLayerBitmapGcode(BitmapHandlerType type) {
  QRectF* bitmap_dirty_area_ = &bitmap_dirty_areas_[type];
  if (bitmap_dirty_area_->width() == 0) return;

  qreal padding_mm;
  qreal accelerate = 4000; // mm/s^2
  switch (padding_type_) {
    case PaddingType::kFixedPadding:
      padding_mm = fixed_padding_mm_;
      break;
    case PaddingType::kDynamicPadding:
      // Adjust padding based on layer speed
      padding_mm = qreal(current_layer_->speed()*current_layer_->speed()) / (2*accelerate); // d = v^2 / 2a
      break;
    case PaddingType::kNoPadding:
    default:
      padding_mm = 0;
      break;
  }
  // NOTE: Express bbox in # of dots
  //       Reserve x-direction padding in bounding box (for acceleration distance)
  const qreal padding_dots = padding_mm * dpmm_;
  const QRectF padded_area =
      bitmap_dirty_area_->adjusted(-padding_dots, 0, padding_dots, 0);
  const QRect work_area{QPoint{0, 0}, canvas_size_.toSize()};
  if (!padded_area.intersects(QRectF{work_area})) {
    // Skip if completely outside of work area
    return;
  }

  // Get the image of entire layer
  QImage layer_image;
  if(with_image_) {
    if (type != BitmapHandlerType::PwmMode) {
      layer_image = layer_bitmaps_[type].toImage()
                        .convertToFormat(QImage::Format_Mono, Qt::MonoOnly | Qt::DiffuseDither)
                        .convertToFormat(QImage::Format_Grayscale8);
    } else {
      layer_image = layer_bitmaps_[type].toImage()
                        .convertToFormat(QImage::Format_Grayscale8);
    }
  } else {
    layer_image = layer_bitmaps_[type].toImage()
                      .convertToFormat(QImage::Format_Grayscale8);
  }

  // NOTE: layer_bitmaps_ is only reallocated when the canvas has to grow, so it
  //       may be larger than the current work area. Clip against both.
  const QRect bbox =
      getImageBBox(padded_area, layer_image.rect().intersected(work_area));
  if (bbox.isEmpty()) {
    return;
  }

  gen_->turnOnLaserAdpatively(); // M4

  // rapid move to the start position
  moveTo(QPointF{bbox.topLeft()} / dpmm_,
         travel_speed_,
         0, 0);

  gen_->useRelativePositioning();

  // Start raster
  switch (type) {
    case BitmapHandlerType::PwmMode:
      rasterBitmapPwmMode(layer_image, bbox, ScanDirectionMode::kBidirectionMode, padding_mm);
      break;
    case BitmapHandlerType::GradientMode:
      gen_->setDottingTime(current_layer_->dottingTime());
      if (is_high_speed_) {
        rasterBitmapHighSpeed(layer_image, bbox, ScanDirectionMode::kBidirectionMode, padding_mm);
      } else {
        std::swap(element_cnt_[0], element_cnt_[1]);
        int count = 0;
        rasterBitmap(layer_image, bbox, ScanDirectionMode::kBidirectionMode, padding_mm, &count);
        gen_->addComment(QString("DOT%1").arg(count));
        std::swap(element_cnt_[0], element_cnt_[1]);
      }
      gen_->setDottingTime(0);
      break;
    default:
      rasterBitmap(layer_image, bbox, ScanDirectionMode::kBidirectionMode, padding_mm);
      break;
  }

  gen_->useAbsolutePositioning();

  gen_->turnOffLaser();
}

/**
 *
 * @param layer_image the (scaled) bitmap of entire layer
 * @param bbox bounding box of dirty area
 *             NOTE: the bbox has already been scaled by dpmm
 * @param diection_mode
 * @return
 */
bool ToolpathExporter::rasterBitmap(const QImage& layer_image,
                                    QRect bbox,
                                    ScanDirectionMode direction_mode,
                                    qreal padding_mm,
                                    int* count,
                                    QPointF offset,
                                    bool should_transpose) {
  int current_grayscale = WHITE_PIXEL; // 0-255 from dark (black) to bright (white)
  bool reverse_raster_dir = false;

  // Prepare raster line paths
  QList<QLine> raster_lines = makeRasterLines(bbox);
  qInfo() << "bbox: " << bbox;
  qInfo() << "# of raster line: " << raster_lines.size();

  float progress_unit = 0.0095 * element_cnt_[0] / total_element_cnt_;
  int progress_batch = raster_lines.size() / 100 + 1;
  int cnt = -1;

  // 2-2. iterate
  for (const auto &raster_line: raster_lines) {
    if (this->cancelled_) {
      return false;
    }
    if (++cnt % progress_batch == 0) {
      onProgressChanged(progress_unit, false);
    }
    // Initialize
    if (raster_line.isNull()) {
      continue;
    }
    std::vector<std::bitset<32>> dot_data_list;
    // TBD: Consider encapsulate the following into a class/struct
    const QPoint initial_pos = reverse_raster_dir ?
                               raster_line.p2() :
                               raster_line.p1();
    const QPoint end_pos = reverse_raster_dir ?
                           raster_line.p1() :
                           raster_line.p2();
    const QLineF path{initial_pos, end_pos};
    const qreal t_step = 1 / path.length();
    qreal current_t_sample = t_step / 2; // NOTE: Offset by dot_size/2 to get the nearest pixel value
    QPointF current_pos_sample = path.pointAt(current_t_sample) - offset;

    // Scan an entire line of bitmap
    std::bitset<32> data_word = 0;
    uint32_t bit_idx = 31;
    bool blank_line = true; // blank line filled with white pixels entirely
    while (1) {
      const uchar *data_ptr = layer_image.constScanLine(current_pos_sample.y());
      int dot_grayscale = data_ptr[int(current_pos_sample.x())];
      //qInfo() << dot_grayscale;
      if (dot_grayscale < WHITE_PIXEL) {
        if(blank_line) blank_line = false;
        if(count) (*count)++;
        data_word.set(bit_idx);
      } else {
        data_word.reset(bit_idx);
      }

      if (bit_idx == 0) {
        dot_data_list.push_back(data_word);
        data_word.reset();
        bit_idx = 31;
      } else {
        bit_idx--;
      }

      current_t_sample += t_step;
      if ( current_t_sample >= 0.99999) { // terminate condition: Reach end pos (within epsilon distance)
        if (bit_idx != 31) { // Append the remaining partial word
          dot_data_list.push_back(data_word);
        }
        break;
      }
      current_pos_sample = path.pointAt(current_t_sample) - offset;
    } // End of parsing of the raster line

    if (blank_line) {
      continue; // ignore blank line
    }

    auto [ trimmed_bit_array, first_pos_idx, last_pos_idx ] =
    adjustPrefixSuffixZero(dot_data_list, padding_mm * dpmm_);
    // NOTE +1 to the end pos because we move to "the end of the last dot"
    QLineF dirty_line_segment {
            path.pointAt(first_pos_idx * t_step),
            path.pointAt(qMin((last_pos_idx + 1) * t_step, 1.0))
    };

    rasterLine(dirty_line_segment, trimmed_bit_array, should_transpose);

    // Switch scan direction if config
    if (direction_mode == ScanDirectionMode::kBidirectionMode) {
      reverse_raster_dir = !reverse_raster_dir;
    }

  } // end of all raster lines

  return true;
}

bool ToolpathExporter::rasterLine(const QLineF& path,
                                  const std::vector<std::bitset<32>>& data,
                                  bool should_transpose) {
  bool is_emitting = false;
  const qreal t_step = 1 / path.length();
  moveTo(
      should_transpose ? (path.p1() / dpmm_).transposed() : path.p1() / dpmm_,
      travel_speed_, 0, 0);

  int idx = 0;
  while (true) {
    if (t_step * idx >= 1) {
      moveTo(
          should_transpose ? (path.p2() / dpmm_).transposed()
                           : path.p2() / dpmm_,
          current_layer_->speed(), is_emitting ? current_layer_->power() : 0,
          is_emitting && !should_transpose ? current_layer_->xBacklash() : 0);
      break;
    }
    if (int(idx/32) >= data.size()) {
      if (is_emitting) {
        moveTo(should_transpose
                   ? (path.pointAt(t_step * idx) / dpmm_).transposed()
                   : path.pointAt(t_step * idx) / dpmm_,
               current_layer_->speed(),
               current_layer_->power(),
               should_transpose ? 0 : current_layer_->xBacklash());
      }
      moveTo(should_transpose ? (path.p2() / dpmm_).transposed()
                              : path.p2() / dpmm_,
             current_layer_->speed(),
             0, 0);
      break;
    }

    if (is_emitting != data[int(idx/32)][31 - (idx % 32)]) {
      moveTo(
          should_transpose ? (path.pointAt(t_step * idx) / dpmm_).transposed()
                           : path.pointAt(t_step * idx) / dpmm_,
          current_layer_->speed(),
          is_emitting ? current_layer_->power() : 0,
          is_emitting && !should_transpose ? current_layer_->xBacklash() : 0);
      is_emitting = !is_emitting;
    }
    idx++;
  }

  return true;
}

bool ToolpathExporter::rasterBitmapPwmMode(const QImage &layer_image,
    QRect bbox, 
    ScanDirectionMode direction_mode, 
    qreal padding_mm) {

  bool reverse_raster_dir = false;

  // Prepare raster line paths
  QList<QLine> raster_lines = makeRasterLines(bbox);
  qInfo() << "bbox: " << bbox;
  qInfo() << "# of raster line: " << raster_lines.size();

  // Iterate through each raster line
  for (const auto &raster_line: raster_lines) {
    if (raster_line.isNull()) {
      continue;
    }
    
    std::vector<std::array<unsigned char, 32>> grayscale_data_list;
    
    // Determine start and end points based on raster direction
    const QPoint initial_pos = reverse_raster_dir ? raster_line.p2() : raster_line.p1();
    const QPoint end_pos = reverse_raster_dir ? raster_line.p1() : raster_line.p2();
    const QLineF path{initial_pos, end_pos};
    
    // Calculate step size for scanning
    const qreal t_step = 1 / path.length();
    qreal current_t_sample = t_step / 2;
    QPointF current_pos_sample = path.pointAt(current_t_sample);

    // Scan an entire line of bitmap
    std::array<unsigned char, 32> grayscale_array;
    uint32_t array_idx = 0;
    bool blank_line = true;

    while (1) {
      // Get grayscale value of current pixel
      const uchar *data_ptr = layer_image.constScanLine(current_pos_sample.y());
      unsigned char dot_grayscale = data_ptr[int(current_pos_sample.x())];
      
      // Update blank_line flag if a non-white pixel is found
      if (dot_grayscale < WHITE_PIXEL && blank_line) {
        blank_line = false;
      }
      
      // Store grayscale value
      grayscale_array[array_idx] = dot_grayscale;

      // If grayscale_array is full, add it to grayscale_data_list and reset
      if (array_idx == 31) {
        grayscale_data_list.push_back(grayscale_array);
        array_idx = 0;
      } else {
        array_idx++;
      }

      // Move to next sample point
      current_t_sample += t_step;
      if (current_t_sample >= 0.99999) {
        if (array_idx != 0) { // Append the remaining partial array
          grayscale_data_list.push_back(grayscale_array);
        }
        break;
      }
      current_pos_sample = path.pointAt(current_t_sample);
    }

    // Skip processing if the line is entirely blank
    if (blank_line) {
      continue;
    }

    // Trim unnecessary white space and adjust for padding
    auto [trimmed_grayscale_array, first_pos_idx, last_pos_idx] =
    adjustPrefixSuffixZero(grayscale_data_list, padding_mm * dpmm_);
    
    // Calculate the actual line segment to be rasterized
    QLineF dirty_line_segment {
            path.pointAt(first_pos_idx * t_step),
            path.pointAt(qMin((last_pos_idx + 1) * t_step, 1.0))
    };

    // Rasterize the line
    rasterLine(dirty_line_segment, trimmed_grayscale_array);

    // Switch scan direction if bidirectional mode is enabled
    if (direction_mode == ScanDirectionMode::kBidirectionMode) {
      reverse_raster_dir = !reverse_raster_dir;
    }
  }

  return true;
}

int ToolpathExporter::calculatePWMPower(unsigned char grayscale) {
  // Implement your PWM calculation logic here
  // For example, you might want to invert the grayscale value and scale it to your PWM range
  return static_cast<int>((255 - grayscale) * current_layer_->power() / 255);
}

bool ToolpathExporter::rasterLine(const QLineF& path, const std::vector<std::array<unsigned char, 32>>& data) {
  const qreal t_step = 1 / path.length();
  
  // Move to the start of the line
  moveTo(path.p1() / dpmm_,
         travel_speed_,
         0, 0);

  int idx = 0;
  while (true) {
    // Check if we've reached the end of the line
    if (t_step * idx >= 1) {
      moveTo(path.p2() / dpmm_,
             current_layer_->speed(),
             0, 0);
      break;
    }
    
    // Check if we've processed all data
    if (int(idx/32) >= data.size()) {
      moveTo(path.p2() / dpmm_,
             current_layer_->speed(),
             0, 0);
      break;
    }

    // Get grayscale value and calculate PWM power
    unsigned char grayscale = data[int(idx/32)][idx % 32];
    int pwm_power = calculatePWMPower(grayscale);

    // Move to the point with calculated PWM power
    moveTo(path.pointAt(t_step * idx) / dpmm_,
           current_layer_->speed(),
           pwm_power,
           current_layer_->xBacklash());

    idx++;
  }

  return true;
}

/**
 * @brief Export
 * @param layer_image  (scaled) image of an entire layer on canvas
 * @param bbox         the (scaled) bounding box for the raster motion (shouldn't be larger than layer_image)
 *                     NOTE: the bbox has already been scaled by dpmm
 * @return
 */
bool ToolpathExporter::rasterBitmapHighSpeed(const QImage &layer_image,
    QRect bbox,
    ScanDirectionMode direction_mode,
    qreal padding_mm) {

  // 1. Enter fast raster mode
  qreal pixel_size = 1/dpmm_;
  gen_->appendCustomCmd(std::string("D0R") +
        std::string(pixel_size >= 0.2 ? "L" :
                    pixel_size >= 0.1 ? "M" :
                    pixel_size >= 0.05 ? "H" : "U") +
       std::string("\n")
   );
  gen_->addComment(QString("PIXEL SIZE %1").arg(pixel_size));

  // 2. Parsing bitmap data and generate command for each raster line
  bool is_emitting_laser = false;
  bool reverse_raster_dir = false;
  int dot_count = 0;
  int jump_count = 0;

  // 2-1. Prepare raster line paths
  QList<QLine> raster_lines = makeRasterLines(bbox);
  qInfo() << "bbox: " << bbox;
  qInfo() << "# of raster line: " << raster_lines.size();

  float progress_unit = 0.0095 * element_cnt_[1] / total_element_cnt_;
  int progress_batch = raster_lines.size() / 100 + 1;
  int cnt = -1;

  // 2-2. iterate
  for (const auto &raster_line: raster_lines) {
    if (this->cancelled_) {
      return false;
    }
    if (++cnt % progress_batch == 0) {
      onProgressChanged(progress_unit, false);
    }
    // Initialize
    if (raster_line.isNull()) {
      continue;
    }
    std::vector<std::bitset<32>> dot_data_list;
    // TBD: Consider encapsulate the following into a class/struct
    const QPoint initial_pos = reverse_raster_dir ?
                       raster_line.p2() :
                       raster_line.p1();
    const QPoint end_pos = reverse_raster_dir ?
                          raster_line.p1() :
                          raster_line.p2();
    const QLineF path{initial_pos, end_pos};
    const qreal t_step = 1 / path.length();
    qreal current_t_sample = t_step / 2; // NOTE: Offset by dot_size/2 to get the nearest pixel value
    QPointF current_pos_sample = path.pointAt(current_t_sample);

    // Scan an entire line of bitmap
    std::bitset<32> data_word = 0;
    uint32_t bit_idx = 31;
    bool blank_line = true; // blank line filled with white pixels entirely
    while (1) {
      const uchar *data_ptr = layer_image.constScanLine(current_pos_sample.y());
      int dot_grayscale = data_ptr[int(current_pos_sample.x())];
      //qInfo() << dot_grayscale;
      if (dot_grayscale < WHITE_PIXEL) {
        if (blank_line) blank_line = false;
        if (!is_emitting_laser) is_emitting_laser = true;
        dot_count++;
        data_word.set(bit_idx);
      } else {
        if (is_emitting_laser) {
          is_emitting_laser = false;
          jump_count++;
        }
        data_word.reset(bit_idx);
      }

      if (bit_idx == 0) {
        dot_data_list.push_back(data_word);
        data_word.reset();
        bit_idx = 31;
      } else {
        bit_idx--;
      }

      current_t_sample += t_step;
      if ( current_t_sample >= 0.99999) { // terminate condition: Reach end pos (within epsilon distance)
        if (bit_idx != 31) { // Append the remaining partial word
          dot_data_list.push_back(data_word);
        }
        break;
      }
      current_pos_sample = path.pointAt(current_t_sample);
    } // End of parsing of the raster line

    if (blank_line) {
      continue; // ignore blank line
    }

    auto [ trimmed_bit_array, first_pos_idx, last_pos_idx ] =
            adjustPrefixSuffixZero(dot_data_list, padding_mm * dpmm_);
    // NOTE +1 to the end pos because we move to "the end of the last dot"
    QLineF dirty_line_segment {
            path.pointAt(first_pos_idx * t_step),
            path.pointAt(qMin((last_pos_idx + 1) * t_step, 1.0))
    };

    rasterLineHighSpeed(dirty_line_segment, trimmed_bit_array);

    // Switch scan direction if config
    if (direction_mode == ScanDirectionMode::kBidirectionMode) {
      reverse_raster_dir = !reverse_raster_dir;
    }

  } // end of all raster lines

  // 5. Exit fast raster mode
  gen_->appendCustomCmd(std::string("D5\n"));
  gen_->addComment(QString("DOT%1").arg(dot_count));
  gen_->addComment(QString("JUMP%1").arg(jump_count));

  return true;
}

/**
 * @brief Export D1PC[n], D2WXXXXXX, D3FE and D4PL cmd for an entire row
 * @param data row data
 * @return
 */
bool ToolpathExporter::rasterLineHighSpeed(const QLineF& path, const std::vector<std::bitset<32>>& data) {
  // Generate moveTo cmd to the initial position of raster line
  moveTo(path.p1() / dpmm_,
         travel_speed_,
         0, 0);

  // Generate D1PC, D2W, D3FE, D4PL cmd based on the parsed info
  gen_->appendCustomCmd(std::string("D1PC") + std::to_string(32*data.size()) + std::string("\n"));

  // Generate D2W commands 6*32 pixels per D2W cmd
  // WARNING: Shouldn't exceed the cmd line buffer size (e.g. 80) of grbl machine
  std::stringstream ss;
  for (auto i = 0; i < data.size(); i++) {
    if (i % 6 == 0) {
      ss.str(std::string());
      ss << "D2W";
    }
    ss << std::setfill('0') << std::setw(8) << std::hex << std::uppercase << data[i].to_ulong();
    if (i % 6 == 5 || (i == data.size() - 1)) {
      ss << "\n";
      gen_->appendCustomCmd(ss.str());
    }
  }

  gen_->appendCustomCmd(std::string("D3FE\n"));
  gen_->appendCustomCmd(std::string("D4PL\n"));

  // Generate moveTo cmd to the end position of raster line
  moveTo(path.p2() / dpmm_,
         current_layer_->speed(),
         current_layer_->power(),
         current_layer_->xBacklash());

  return true;
}

bool ToolpathExporter::rasterBitmapDepthMode(ScanDirectionMode direction_mode,
                                             qreal padding_mm) {
  gen_->turnOnLaserAdpatively();

  double progress_unit_elem = 0.95 / total_element_cnt_;
  double progress_unit_pass;
  for (const auto& bmp : depth_mode_bitmaps_) {
    if (this->cancelled_) return false;
    QImage bitmap_image =
        bmp->sourceImage()
            .transformed(bmp->transform() * bmp->tempTransform(), Qt::SmoothTransformation)
            .convertToFormat(QImage::Format_ARGB32);

    const QPoint offset = bmp->tempTransform().mapRect(bmp->boundingRect()).topLeft().toPoint();
    const QRect image_rect =
        bitmap_image.rect().translated(offset);
    QRect bbox = getImageBBox(
        QRectF(image_rect), QRect(0, 0, canvas_width_, canvas_height_));
    if (bbox.isEmpty()) continue;

    double zStep = bmp->depthZStep();
    int depthPass = bmp->depthPass();
    int minVal = 0;
    int maxVal = 255;
    bool res = findMinMaxPixel(&bitmap_image, &minVal, &maxVal);
    if (!res) continue;
    qInfo() << "Depth mode min/max pixel value: " << minVal << "/" << maxVal << "with" << depthPass << "passes";
    progress_unit_pass = progress_unit_elem / depthPass;
    double threshold_step = double(maxVal - minVal) / depthPass;
    int threshold = 256, threshold_tr = 256, current_threshold;
    QImage binary_image, bitmap_image_tr, binary_image_tr;
    QRect bbox_tr(bbox.y(), bbox.x(), bbox.height(), bbox.width());
    QPoint offset_tr = offset.transposed();
    bool transposed = false;

    gen_->useAbsolutePositioning();
    moveTo(QPointF{bbox.topLeft()} / dpmm_, travel_speed_, 0, 0);
    gen_->useRelativePositioning();
    for (int i = 0; i < depthPass; i++) {
      if (this->cancelled_) return false;
      current_threshold = (i == 0 && depthPass <= 1)
                              ? (maxVal + minVal) / 2
                              : (maxVal - i * threshold_step);
      if (i != 0 && zStep != 0) {
        gen_->moveZ(-zStep);
      }
      if (transposed) {
        if (current_threshold != threshold_tr) {
          if (current_threshold == threshold) {
            binary_image_tr = imageTranspose(&binary_image);
          } else {
            if (bitmap_image_tr.isNull()) {
              bitmap_image_tr = imageTranspose(&bitmap_image);
            }
            binary_image_tr = imageBinarize(&bitmap_image_tr, current_threshold);
          }
          threshold_tr = current_threshold;
        }
        rasterBitmap(binary_image_tr, bbox_tr, direction_mode, padding_mm, nullptr, offset_tr, transposed);
      } else {
        if (current_threshold != threshold) {
          if (current_threshold == threshold_tr) {
            binary_image = imageTranspose(&binary_image_tr);
          } else {
            binary_image = imageBinarize(&bitmap_image, current_threshold);
          }
          threshold = current_threshold;
        }
        rasterBitmap(binary_image, bbox, direction_mode, padding_mm, nullptr, offset, transposed);
      }
      transposed = !transposed;
      onProgressChanged(progress_unit_pass, false);
    }
    if (depthPass >= 0 && zStep != 0) {
      gen_->moveZ(zStep * (depthPass - 1));
    }
  }
  gen_->useAbsolutePositioning();
  gen_->turnOffLaser();
  return true;
}

inline void ToolpathExporter::moveTo(QPointF&& dest, double speed, double power, double x_backlash) {
  gen_->moveTo(dest.x(), dest.y(), speed, power, enable_custom_backlash_ ? x_backlash : 0);
  current_pos_mm_ = dest;
}

inline void ToolpathExporter::moveTo(const QPointF& dest, double speed, double power, double x_backlash) {
  gen_->moveTo(dest.x(), dest.y(), speed, power, enable_custom_backlash_ ? x_backlash : 0);
  current_pos_mm_ = dest;
}

void ToolpathExporter::handleCancel() {
  this->cancelled_ = true;
}

/**
 * Update progress and emit signal if necessary
 * Also check if the process is cancelled
 */
void ToolpathExporter::onProgressChanged(double value, bool absolute) {
  current_progress_ = absolute ? value : current_progress_ + value * progress_increment_scale_;
  int new_progress = 100 * (processed_layer_cnt_ + (processed_repeat_times_ + current_progress_) / total_repeat_times_) / total_layer_cnt_;
  if (new_progress > progress_) {
    progress_ = new_progress;
    Q_EMIT progressChanged(progress_);
  }
  // Always call processEvents to receive the cancellation signal quickly
  QCoreApplication::processEvents();
}
