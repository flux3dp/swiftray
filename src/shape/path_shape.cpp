#include <shape/path_shape.h>
#include <QDebug>

#define SELECTION_TOLERANCE 10

using namespace std;

PathShape::PathShape() noexcept {
    
}

PathShape::PathShape(QPainterPath path) : Shape() {
    path_ = path;
    
    // Realign path to center in local coord 0, 0
    QRectF bbox = path_.boundingRect();
    path_ = QTransform()
            .translate(-bbox.center().x(), -bbox.center().y())
            .map(path_);
    // Set global coord of this object to original path center
    setTransform(transform().translate(bbox.center().x(), bbox.center().y()));
    calcBoundingBox();
}


PathShape::~PathShape() {
}

bool PathShape::hitTest(QPointF global_coord, qreal tolerance) {
    QPointF local_coord = transform().inverted().map(global_coord);

    if (!hit_test_rect_.contains(local_coord)) {
        return false;
    }

    return hitTest(QRectF(global_coord.x() - tolerance, global_coord.y() - tolerance, tolerance * 2, tolerance * 2));
}

bool PathShape::hitTest(QRectF global_coord_rect) {
    QPainterPath new_path = transform().map(path_);
    // TODO:: Logic still has bug when the path is not closed
    return new_path.intersects(global_coord_rect) && !new_path.contains(global_coord_rect);
}

void PathShape::calcBoundingBox() {
    bbox_ = transform().map(path_).boundingRect();
    QRectF local_bbox = path_.boundingRect(); //todo merge bbox_ with local_bbox
    hit_test_rect_ = QRectF(local_bbox.x() - SELECTION_TOLERANCE, local_bbox.y() - SELECTION_TOLERANCE, 
                            local_bbox.width() + SELECTION_TOLERANCE * 2, local_bbox.height() + SELECTION_TOLERANCE * 2);
}

void PathShape::paint(QPainter *painter) {
    painter->save();
    painter->setTransform(transform(), true);
    painter->drawPath(path_);
    painter->restore();
}

shared_ptr<Shape> PathShape::clone() const {
    shared_ptr<PathShape> shape = make_shared<PathShape>(*this);
    return shape;
}

Shape::Type PathShape::type() const {
    return Shape::Type::Path;
}

QPainterPath& PathShape::path(){
    return path_;
}

void PathShape::setPath(QPainterPath &path) {
    // Input path coord is local coord
    path_ = path;
    calcBoundingBox();
}