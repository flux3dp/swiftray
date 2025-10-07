#include <toolpath_exporter/convex-hull-exporter.h>
#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>

/**
 * 1. Compute the convex hull of a set of points
 * 2. Limit the convex hull inside the work area
 * 3. Update global exceed_boundary_ flag
 * 4. Return bounded hull
 */
QPolygonF ConvexHullExporter::computeConvexHull(QPolygonF& points) {
  cv::Mat pts = QPolygonToMat(points);
  cv::Mat hull;
  cv::Mat bounded_hull;

  cv::convexHull(pts, hull, true, true);
  cv::Rect bbox = cv::boundingRect(hull);
  bool exceeding_boundary =
      bbox.x < 0 || bbox.y < 0 ||
      bbox.x + bbox.width > machine_work_area_mm_.width() ||
      bbox.y + bbox.height > machine_work_area_mm_.height();
  if (exceeding_boundary) {
    exceed_boundary_ = true;
    cv::intersectConvexConvex(hull, machine_work_area_mat_, bounded_hull, true);
    return MatFToQPolygon(bounded_hull);
  }
  return MatFToQPolygon(hull);
}

ConvexHullExporter::ConvexHullExporter(BaseGenerator* generator) noexcept
    : gen_(generator) {
  // convert px to mm
  global_transform_ = QTransform::fromScale(0.1, 0.1);
}

bool ConvexHullExporter::convertStack(const QList<LayerPtr>& layers) {
  qInfo() << "[Export] Start converting stack with layers" << layers.count();
  progress_ = 0;
  QElapsedTimer t;
  t.start();
  // Initial Setup
  gen_->disableRotary();
  gen_->setRedLight(true);
  gen_->turnOffLaser();
  gen_->useAbsolutePositioning();
  gen_->setWorkarea(machine_work_area_mm_);
  qInfo() << "[Export] Workarea size: " << machine_work_area_mm_;

  // Start Parsing Layers
  qInfo() << "[Export] Start parsing layers";
  Q_ASSERT_X(!layers.empty(), "ConvexHullExporter", "Must input at least one layer");

  processed_layer_cnt_ = 0;
  total_layer_cnt_ = layers.size();
  gen_->turnOnLaser(); // M3
  for (auto layer_rit = layers.crbegin(); layer_rit != layers.crend(); layer_rit++) {
    onProgressChanged(0, true);
    if ((*layer_rit)->isVisible() && (*layer_rit)->repeat() > 0) {
      qInfo() << "[Export] Output layer: " << (*layer_rit)->name();
      convertLayer((*layer_rit));
    }
    if (this->cancelled_) {
      break;
    }
    processed_layer_cnt_++;
  }

  if (this->cancelled_) {
    return false;
  }

  gen_->finishProgramFlow();

  if (this->cancelled_) {
    return false;
  }

  qInfo() << "[Export] Took " << t.elapsed() << " milliseconds";
  return true;
}

void ConvexHullExporter::convertLayer(const LayerPtr& layer) {
  current_transform_ = global_transform_;
  polygons_mutex_.lock();
  hulls_.clear();
  polygons_mutex_.unlock();

  QPolygonF element_path_;
  double element_progress = 0.1 / layer->children().size();
  for (auto& shape : layer->children()) {
    element_path_ = convertShape(shape);
    if (!element_path_.isEmpty()) {
      QPolygonF hull = computeConvexHull(element_path_);
      if (hull.isEmpty()) {
        continue;
      }
      polygons_mutex_.lock();
      hulls_.append(hull);
      polygons_mutex_.unlock();
    }

    if (this->cancelled_) {
      return;
    }
    onProgressChanged(element_progress, false);
  }

  onProgressChanged(0.10, true);
  outputLayerGcode();
  onProgressChanged(1, true);
}

QPolygonF ConvexHullExporter::convertShape(const ShapePtr& shape) {
  switch (shape->type()) {
    case Shape::Type::Group:
      return convertGroup(dynamic_cast<GroupShape*>(shape.get()));
    case Shape::Type::Bitmap:
      return convertBitmap(dynamic_cast<BitmapShape*>(shape.get()));
    case Shape::Type::Path:
    case Shape::Type::Text:
      return convertPath(dynamic_cast<PathShape*>(shape.get()));
    default:
      return QPolygonF();
  }
}

QPolygonF ConvexHullExporter::convertGroup(const GroupShape* group) {
  current_transform_ = group->globalTransform() * global_transform_;
  QPolygonF group_polygon;
  for (auto& shape : group->children()) {
    group_polygon += convertShape(shape);
  }
  current_transform_ = global_transform_;
  return group_polygon;
}

QPolygonF ConvexHullExporter::convertBitmap(const BitmapShape* bmp) {
  return current_transform_.map(bmp->boundingRect());
}

QPolygonF ConvexHullExporter::convertPath(const PathShape* path) {
  QPainterPath transformed_path = (path->transform() * current_transform_).map(path->path());
  return transformed_path.toFillPolygon();
}

void ConvexHullExporter::outputLayerGcode() {
  int i = 0;
  double element_progress = 0.9 / hulls_.size();

  polygons_mutex_.lock();
  for (auto& hull : hulls_) {
    for (i = 0; i < repeat_; i++) {
      for (QPointF& point : hull) {
        moveTo(point);
      }
    }
    moveTo(hull.first());
    if (this->cancelled_) {
      break;
    }
    onProgressChanged(element_progress, false);
  }
  polygons_mutex_.unlock();
}

void ConvexHullExporter::moveTo(QPointF p) {
  // Note: speed is not used in Promark framing
  gen_->moveTo(p.x(), p.y(), 4000, 0, 0);
}

void ConvexHullExporter::setWorkAreaSize(QRectF work_area) {
  machine_work_area_mm_ = work_area;
  machine_work_area_mat_ = QPolygonToMat(QPolygonF(work_area));
}

void ConvexHullExporter::handleCancel() {
  this->cancelled_ = true;
}

/**
 * Update progress and emit signal if necessary
 * Also check if the process is cancelled
 */
void ConvexHullExporter::onProgressChanged(double value, bool absolute) {
  current_progress_ = absolute ? value : current_progress_ + value;
  int new_progress = 100 * (processed_layer_cnt_ + current_progress_) / total_layer_cnt_;
  if (new_progress > progress_) {
    progress_ = new_progress;
    Q_EMIT progressChanged(progress_);
  }
  // Always call processEvents to receive the cancellation signal quickly
  QCoreApplication::processEvents();
}
