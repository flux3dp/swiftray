#include "shape.hpp"
#include <QDebug>

#define SELECTION_TOLERANCE 15

Shape::Shape() noexcept {
    x = 0;
    y = 0;
    rot = 0;
    scaleX = 1;
    scaleY = 1;
    selected = false;
}

void Shape::simplify() {
    //Realign object bounding box;
    QRectF bbox = path.boundingRect();
    x = x + bbox.x() + bbox.width() / 2;
    y = y + bbox.y() + bbox.height() / 2;
    path.translate(-bbox.x() - bbox.width() / 2, -bbox.y() - bbox.height() / 2);
    // Caching paths to points for selection testing
    cacheSelectionTestingData();
}

void Shape::cacheSelectionTestingData() {
    QRectF bbox = path.boundingRect();
    float pathLength = path.length();
    float segStep = 15 / pathLength; // 15 units per segments

    for (float percent = 0; percent < 1; percent += segStep) {
        selectionTestingPoints << path.pointAtPercent(percent);
    }

    selectionTestingPoints << path.pointAtPercent(1);
    selectionTestingRect = QRectF(bbox.x() - SELECTION_TOLERANCE, bbox.y() - SELECTION_TOLERANCE, bbox.width() + SELECTION_TOLERANCE * 2, bbox.height() + SELECTION_TOLERANCE * 2);
    // qInfo() << "Selection box" << selectionTestingRect;
}

QPointF Shape::pos() {
    return QPointF(x, y);
}


bool Shape::testHit(QPointF globalCoord) {
    //Rotate and scale global coord to local coord
    QPointF d = globalCoord - this->pos();
    float newTheta = atan2(d.y(), d.x()) - this->rot * 3.1415926 / 180;
    QPointF localCoord = sqrt(d.x() * d.x() + d.y() * d.y()) * QPointF(cos(newTheta), sin(newTheta));
    localCoord = QPointF(localCoord.x() * this->scaleX, localCoord.y() * this->scaleY);
    qInfo() << "Local coord" << localCoord;

    if (!selectionTestingRect.contains(localCoord)) {
        return false;
    }

    qInfo() << "Hit bbox" << localCoord;

    for (int i = 0; i < selectionTestingPoints.size(); i++) {
        if ((selectionTestingPoints[i] - localCoord).manhattanLength() < SELECTION_TOLERANCE) {
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
           .scale(this->scaleX, this->scaleY)
           .map(orig).boundingRect();
}
