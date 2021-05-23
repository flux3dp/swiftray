#include "shape.hpp"
#include <QDebug>

void Shape::simplify() {
    selected = true;
    //Realign object bounding box;
    QRectF bbox = path.boundingRect();
    x = bbox.x();
    y = bbox.y();
    path.translate(-x, -y);
    // Caching paths to points for selection testing
    float pathLength = path.length();
    float segStep = 15 / pathLength; // 15 units per segments

    for (float percent = 0; percent < 1; percent += segStep) {
        polyCache << path.pointAtPercent(percent);
    }

    polyCache << path.pointAtPercent(1);
}

bool Shape::testHit(QPointF point) {
    QPointF offset(x, y);

    for (int i = 0; i < polyCache.size(); i++) {
        if ((polyCache[i] + offset - point).manhattanLength() < 15) {
            return true;
        }
    }

    return false;
}
