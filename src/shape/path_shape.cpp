#include "path_shape.h"
#include <QDebug>

#define SELECTION_TOLERANCE 10

using namespace std;

PathShape::PathShape(QPainterPath path) : Shape() {
    path_ = path;
}


PathShape::~PathShape() {
    qInfo() << "PathShape::Deconstruct" << this;
}

// todo: add setPath and avoid externally changing the paths.
void PathShape::simplify() {
    // Realign object bounding box;
    QRectF bbox = path_.boundingRect();
    path_ = QTransform()
            .translate(-bbox.center().x(), -bbox.center().y())
            .map(path_);
    // Set x,y
    setTransform(transform().translate(bbox.center().x(), bbox.center().y()));
    // Caching paths to points for selection testing
    cacheSelectionTestingData();
}

// only calls this when the path is different
void PathShape::cacheSelectionTestingData() {
    QRectF bbox = path_.boundingRect();

    if (path_.elementCount() < 200) {
        float path_length = path_.length();
        float seg_step = 15 / path_length; // 15 units per segments

        for (float percent = 0; percent < 1; percent += seg_step) {
            selection_testing_points_ << path_.pointAtPercent(percent);
        }

        selection_testing_points_ << path_.pointAtPercent(1);
    } else {
        QList<QPolygonF> polygons = path_.toSubpathPolygons();

        for (int i = 0; i < polygons.size(); i++) {
            selection_testing_points_.append(polygons[i].toList());
        }
    }

    // Todo:: Add tolerance here
    selection_testing_rect_ = QRectF(bbox.x() - SELECTION_TOLERANCE, bbox.y() - SELECTION_TOLERANCE, bbox.width() + SELECTION_TOLERANCE * 2, bbox.height() + SELECTION_TOLERANCE * 2);
}

bool PathShape::testHit(QPointF global_coord, qreal tolerance) const {
    //Rotate and scale global coord to local coord
    QPointF local_coord = transform().inverted().map(global_coord);

    if (!selection_testing_rect_.contains(local_coord)) {
        return false;
    }

    for (int i = 0; i < selection_testing_points_.size(); i++) {
        if ((selection_testing_points_[i] - local_coord).manhattanLength() < tolerance) {
            return true;
        }
    }

    return false;
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
    qInfo() << "Clone PathShape: ref=" << shape.use_count();
    return shape;
}

