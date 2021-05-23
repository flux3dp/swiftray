#include "shape.hpp"

void Shape::simplify() {
    QRectF bbox = path.boundingRect();
    x = bbox.x();
    y = bbox.y();
    path.translate(-x, -y);
}
