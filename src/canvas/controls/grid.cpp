#include <QPainterPath>
#include <canvas/controls/grid.h>
#include <windows/osxwindow.h>
#include <shape/path-shape.h>

using namespace Controls;

bool Grid::isActive() {
  return true;
}

void Grid::paint(QPainter *painter) {
  painter->fillRect(0, 0, document().width(), document().height(),
                    isDarkMode() ? QColor("#F0F0F0") : Qt::white);
  QPen grey_pen(QColor("#DDD"), line_width_/2., Qt::SolidLine);
  QPen black_pen(QColor("#333"), line_width_/2., Qt::SolidLine);
  QPen black_thick_pen(QColor("#333"), line_width_, Qt::SolidLine);
  grey_pen.setCosmetic(true);
  black_pen.setCosmetic(true);
  black_thick_pen.setCosmetic(true);

  painter->setPen(grey_pen);
  for (int x = 0; x < document().width(); x += 100) {
    painter->drawLine(x, 0, x, document().height());
  }
  for (int y = 0; y < document().height(); y += 100) {
    painter->drawLine(0, y, document().width(), y);
  }

  painter->setPen(black_pen);
  for (int x = 0; x < document().width(); x += 1000) {
    painter->drawLine(x, 0, x, document().height());
  }
  for (int y = 0; y < document().height(); y += 1000) {
    painter->drawLine(0, y, document().width(), y);
  }

  painter->setPen(black_thick_pen);
  painter->drawRect(0, 0, document().width(), document().height());
}