#include <QDebug>
#include <shape/path-shape.h>

#define SELECTION_TOLERANCE 10

using namespace std;

PathShape::PathShape() noexcept: Shape(), filled_(false) {}

PathShape::PathShape(QPainterPath path) : Shape(), filled_(false) {
  path_ = path;

  // Realign path to center in local coord 0, 0
  QRectF bbox = path_.boundingRect();
  path_ = QTransform()
       .translate(-bbox.center().x(), -bbox.center().y())
       .map(path_);
  // Set global coord of this object to original path center
  setTransform(QTransform().translate(bbox.center().x(), bbox.center().y()));
  calcBoundingBox();
}

PathShape::~PathShape() {}

// TODO (Add hitTest for filling mode)
bool PathShape::hitTest(QPointF global_coord, qreal tolerance) const {
  QPointF local_coord = transform().inverted().map(global_coord);

  if (!hit_test_rect_.contains(local_coord)) {
    return false;
  }

  return hitTest(QRectF(global_coord.x() - tolerance,
                        global_coord.y() - tolerance, tolerance * 2,
                        tolerance * 2));
}

bool PathShape::hitTest(QRectF global_coord_rect) const {
  // TODO (Test revese transform to map rect back to local coord)
  QPainterPath new_path = transform().map(path_);
  // TODO (Consider cases that the path is not closed)
  return new_path.intersects(global_coord_rect) &&
         !new_path.contains(global_coord_rect);
}

void PathShape::calcBoundingBox() const {
  QRectF local_bbox = path_.boundingRect();
  this->bbox_ = transform().map(path_).boundingRect();
  this->rotated_bbox_ = transform().map(QPolygonF(local_bbox));
  this->hit_test_rect_ =
       QRectF(local_bbox.x() - SELECTION_TOLERANCE,
              local_bbox.y() - SELECTION_TOLERANCE,
              local_bbox.width() + SELECTION_TOLERANCE * 2,
              local_bbox.height() + SELECTION_TOLERANCE * 2);
}

void PathShape::paint(QPainter *painter) const {
  painter->save();
  if (selected_) painter->setTransform(temp_transform_, true);
  painter->setTransform(transform(), true);
  painter->drawPath(path_);
  painter->restore();
}

shared_ptr<Shape> PathShape::clone() const {
  shared_ptr<PathShape> shape = make_shared<PathShape>(*this);
  return shape;
}

Shape::Type PathShape::type() const { return Shape::Type::Path; }

const QPainterPath &PathShape::path() const { return path_; }

void PathShape::setPath(const QPainterPath &path) {
  // Input path coord is local coord
  path_ = path;
  flushCache();
}

bool PathShape::isFilled() const {
  return filled_;
}

void PathShape::setFilled(bool filled) {
  qInfo() << "Filled";
  filled_ = filled;
}
