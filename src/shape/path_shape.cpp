#include <shape/path_shape.h>
#include <QDebug>

#define SELECTION_TOLERANCE 10

using namespace std;

PathShape::PathShape() noexcept {
    
}

PathShape::PathShape(QPainterPath path) : Shape() {
    path_ = path;
    
    // Realign object to center
    QRectF bbox = path_.boundingRect();
    path_ = QTransform()
            .translate(-bbox.center().x(), -bbox.center().y())
            .map(path_);
    // Set x,y to center
    setTransform(transform().translate(bbox.center().x(), bbox.center().y()));

    simplify();
}


PathShape::~PathShape() {
}

void PathShape::simplify() {
    // Caching paths to points for selection testing
    cacheSelectionTestingData();
}

// only calls this when the path is different
void PathShape::cacheSelectionTestingData() {
    QRectF bbox = path_.boundingRect();
    selection_testing_rect_ = QRectF(bbox.x() - SELECTION_TOLERANCE, bbox.y() - SELECTION_TOLERANCE, bbox.width() + SELECTION_TOLERANCE * 2, bbox.height() + SELECTION_TOLERANCE * 2);
}

bool PathShape::testHit(QPointF global_coord, qreal tolerance) const {
    //Rotate and scale global coord to local coord
    QPointF local_coord = transform().inverted().map(global_coord);

    if (!selection_testing_rect_.contains(local_coord)) {
        return false;
    }

    return testHit(QRectF(global_coord.x() - tolerance, global_coord.y() - tolerance, tolerance * 2, tolerance * 2));
}

bool PathShape::testHit(QRectF global_coord_rect) const {
    QPainterPath new_path = transform().map(path_);
    // TODO:: Logic still has bug when the path is not closed
    return new_path.intersects(global_coord_rect) && !new_path.contains(global_coord_rect);
}

QRectF PathShape::boundingRect() const {
    QRectF origRect = path_.boundingRect();
    QPolygonF orig;
    orig << origRect.topLeft() << origRect.topRight() << origRect.bottomRight() << origRect.bottomLeft();
    return transform().map(path_).boundingRect();
}

void PathShape::paint(QPainter *painter) const {
    painter->save();
    painter->setTransform(transform(), true);
    painter->drawPath(path_);
    painter->restore();
}

shared_ptr<Shape> PathShape::clone() const {
    shared_ptr<PathShape> shape(new PathShape(*this));
    return shape;
}

Shape::Type PathShape::type() const {
    return Shape::Type::Path;
}

QPainterPath& PathShape::path(){
    return path_;
}

void PathShape::setPath(QPainterPath &path) {
    path_ = path;
    simplify();
}