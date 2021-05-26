#include "shape.hpp"
#include <QDebug>

#define SELECTION_TOLERANCE 15

Shape::Shape() noexcept {
    transform_ = QTransform();
    selected = false;
}
// todo: add setPath and avoid externally changing the paths.
void Shape::simplify() {
    // Realign object bounding box;
    QRectF bbox = path.boundingRect();
    path = QTransform()
           .translate(-bbox.center().x(), -bbox.center().y())
           .map(path);
    // Set x,y
    transform_ = transform_.translate(bbox.center().x(), bbox.center().y());
    // Caching paths to points for selection testing
    cacheSelectionTestingData();
}

// only calls this when the path is different
void Shape::cacheSelectionTestingData() {
    QRectF bbox = path.boundingRect();

    if (path.elementCount() < 200) {
        float path_length = path.length();
        float seg_step = 15 / path_length; // 15 units per segments

        for (float percent = 0; percent < 1; percent += seg_step) {
            selection_testing_points << path.pointAtPercent(percent);
        }

        selection_testing_points << path.pointAtPercent(1);
    } else {
        QList<QPolygonF> polygons = path.toSubpathPolygons();

        for (int i = 0; i < polygons.size(); i++) {
            selection_testing_points.append(polygons[i].toList());
        }
    }

    selection_testing_rect = QRectF(bbox.x() - SELECTION_TOLERANCE, bbox.y() - SELECTION_TOLERANCE, bbox.width() + SELECTION_TOLERANCE * 2, bbox.height() + SELECTION_TOLERANCE * 2);
    // qInfo() << "Selection box" << selectionTestingRect;
}

qreal Shape::x() const {
    return transform_.dx();
}

qreal Shape::y() const {
    return transform_.dy();
}

qreal Shape::scaleX() const {
    return transform_.m22();
}

qreal Shape::scaleY() const {
    return transform_.m23();
}

QPointF Shape::pos() const {
    return QPointF(x(), y());
}

void Shape::transform(QTransform transform) {
    transform_ = transform_ * transform;
}

void Shape::pretransform(QTransform transform) {
    transform_ =  transform * transform_;
}

void Shape::translate(qreal x, qreal y) {
    transform_.translate(x, y);
}

bool Shape::testHit(QPointF global_coord, qreal tolerance) const {
    //Rotate and scale global coord to local coord
    QPointF local_coord = transform_.inverted().map(global_coord);

    if (!selection_testing_rect.contains(local_coord)) {
        return false;
    }

    for (int i = 0; i < selection_testing_points.size(); i++) {
        if ((selection_testing_points[i] - local_coord).manhattanLength() < tolerance) {
            return true;
        }
    }

    return false;
}

QTransform Shape::globalTransform() const {
    return transform_;
}

bool Shape::testHit(QRectF global_coord_rect) const {
    QPainterPath new_path = transform_.map(path);
    // TODO:: Logic still has bug when the path is not closed
    return new_path.intersects(global_coord_rect) && !new_path.contains(global_coord_rect);
}

QRectF Shape::boundingRect() const {
    QRectF origRect = path.boundingRect();
    QPolygonF orig;
    orig << origRect.topLeft() << origRect.topRight() << origRect.bottomRight() << origRect.bottomLeft();
    return transform_.map(path).boundingRect();
}
