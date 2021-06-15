#include <QPainterPath>
#include <canvas/controls/grid.h>
#include <cmath>
#include <shape/path-shape.h>

using namespace Controls;

bool Grid::isActive() {
  return true;
}

void Grid::paint(QPainter *painter) {
  painter->fillRect(0, 0, scene().width(), scene().height(), Qt::white);
  QPen grey_pen(QColor("#DDD"), 1, Qt::SolidLine);
  QPen black_pen(Qt::black, 1, Qt::SolidLine);
  QPen black_thick_pen(Qt::black, 2, Qt::SolidLine);
  grey_pen.setCosmetic(true);
  black_pen.setCosmetic(true);
  black_thick_pen.setCosmetic(true);

  painter->setPen(grey_pen);
  for (int x = 0; x < scene().width(); x += 100) {
    painter->drawLine(x, 0, x, scene().height());
  }
  for (int y = 0; y < scene().height(); y += 100) {
    painter->drawLine(0, y, scene().width(), y);
  }

  painter->setPen(black_pen);
  for (int x = 0; x < scene().width(); x += 1000) {
    painter->drawLine(x, 0, x, scene().height());
  }
  for (int y = 0; y < scene().height(); y += 1000) {
    painter->drawLine(0, y, scene().width(), y);
  }

  painter->setPen(black_thick_pen);
  painter->drawRect(0, 0, scene().width(), scene().height());
}