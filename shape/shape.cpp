#include "shape.hpp"
#include <QDebug>

#define SELECTION_TOLERANCE 15

Shape::Shape() noexcept {
    x = 0;
    y = 0;
    rot = 0;
    scale_x = 1;
    scale_y = 1;
    selected = false;
}
// todo: add setPath and avoid externally changing the paths.
void Shape::simplify() {
    //Realign object bounding box;
    QRectF bbox = path.boundingRect();
    x = x + bbox.x() + bbox.width() / 2;
    y = y + bbox.y() + bbox.height() / 2;
    path.translate(-bbox.x() - bbox.width() / 2, -bbox.y() - bbox.height() / 2);
    // Caching paths to points for selection testing
    cacheSelectionTestingData();
}

// only calls this when the path is different
void Shape::cacheSelectionTestingData() {
    QRectF bbox = path.boundingRect();
    float path_length = path.length();
    float seg_step = 15 / path_length; // 15 units per segments

    for (float percent = 0; percent < 1; percent += seg_step) {
        selection_testing_points << path.pointAtPercent(percent);
    }

    selection_testing_points << path.pointAtPercent(1);
    selection_testing_rect = QRectF(bbox.x() - SELECTION_TOLERANCE, bbox.y() - SELECTION_TOLERANCE, bbox.width() + SELECTION_TOLERANCE * 2, bbox.height() + SELECTION_TOLERANCE * 2);
    // qInfo() << "Selection box" << selectionTestingRect;
}

QPointF Shape::pos() {
    return QPointF(x, y);
}


bool Shape::testHit(QPointF global_coord) {
    //Rotate and scale global coord to local coord
    QPointF d = global_coord - this->pos();
    float newTheta = atan2(d.y(), d.x()) - this->rot * 3.1415926 / 180;
    QPointF local_coord = sqrt(d.x() * d.x() + d.y() * d.y()) * QPointF(cos(newTheta), sin(newTheta));
    local_coord = QPointF(local_coord.x() * this->scale_x, local_coord.y() * this->scale_y);
    qInfo() << "Local coord" << local_coord;

    if (!selection_testing_rect.contains(local_coord)) {
        return false;
    }

    qInfo() << "Hit bbox" << local_coord;

    for (int i = 0; i < selection_testing_points.size(); i++) {
        if ((selection_testing_points[i] - local_coord).manhattanLength() < SELECTION_TOLERANCE) {
            qInfo() << "Selected";
            return true;
        }
    }

    return false;
}

QRectF Shape::boundingRect() {
    QRectF origRect = path.boundingRect();
    QPolygonF orig;
    orig << origRect.topLeft() << origRect.topRight() << origRect.bottomLeft() << origRect.bottomRight();
    return QTransform()
           .translate(x, y)
           .rotate(this->rot)
           .scale(this->scale_x, this->scale_y)
           .map(orig).boundingRect();
}
