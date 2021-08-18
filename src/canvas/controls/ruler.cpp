#include <canvas/controls/ruler.h>
#include <windows/osxwindow.h>
#include <shape/path-shape.h>
#include <QPainterPath>
#include <canvas/canvas.h>
#include <QPainter>


using namespace Controls;

bool Ruler::isActive() {
  return true;
}

qreal Ruler::getScaleStep() {
  qreal step = 1000; // 100 mm
  qreal result;
  while (step * document().scale() > 20) {
    result = step / 2;
    if (result * document().scale() < 20) {
      return result;
    }
    result = step / 5;
    if (result * document().scale() < 20) {
      return result;
    }
    step /= 10;
  }
  return step;
}


void Ruler::drawHorizontalRuler(QPainter *painter, qreal step, int thickness,
                                const QPen& line_pen, const QColor& ruler_color) {

  painter->fillRect(QRectF{double(thickness), 0, canvas().width() - thickness, double(thickness)}, ruler_color);

  painter->setPen(line_pen);
  qreal x = document().scroll().x();
  for (int i = 0;; i++) {
    if (x > canvas().width()) {
      break;
    }
    int x_in_pixel = round(x);
    if (i % 10 == 0) {
      painter->drawText(QPointF{double(x_in_pixel+5),  10}, QString::number(i * step / 10));
      painter->drawLine(x_in_pixel, thickness, x_in_pixel, 0);
    } else if (i % 2 == 0) {
      painter->drawLine(x_in_pixel, thickness, x_in_pixel, thickness - 10);
    } else {
      painter->drawLine(x_in_pixel, thickness, x_in_pixel, thickness - 5);
    }
    x += step * document().scale();
  }
  x = document().scroll().x();
  for (int i = 0;; i--) {
    if (x < 0) {
      break;
    }
    int x_in_pixel = round(x);
    if (i % 10 == 0) {
      painter->drawText(QPointF{double(x_in_pixel+5),  10}, QString::number(i * step / 10));
      painter->drawLine(x_in_pixel, thickness, x_in_pixel, 0);
    } else if (i % 2 == 0) {
      painter->drawLine(x_in_pixel, thickness, x_in_pixel, thickness - 10);
    } else {
      painter->drawLine(x_in_pixel, thickness, x_in_pixel, thickness - 5);
    }
    x -= step * document().scale();
  }

  painter->drawLine(thickness, thickness, canvas().width(), thickness);
}
void Ruler::drawVerticalRuler(QPainter *painter, qreal step, int thickness,
                              const QPen& line_pen, const QColor& ruler_color) {

  auto TextToVertical = [](const QString &str) {
    QString result;
    for (auto it = str.constBegin(); it != str.constEnd(); it++) {
      result += *it;
      result += "\n";
    }
    return result;
  };

  painter->fillRect(QRectF{0, double(thickness), double(thickness), canvas().height() - thickness}, ruler_color);

  painter->setPen(line_pen);
  qreal y = document().scroll().y();
  for (int i = 0;; i++) {
    if (y > canvas().height()) {
      break;
    }
    int y_in_pixel = round(y);
    if (i % 10 == 0) {
      QString verticalValue = TextToVertical(QString::number(i * step / 10));
      QRect rect{1, y_in_pixel + 5, 10, 100};
      painter->drawText(rect, 0, verticalValue);
      painter->drawLine(thickness, y_in_pixel, 0, y_in_pixel);
    } else if (i % 2 == 0) {
      painter->drawLine(thickness, y_in_pixel, thickness - 10, y_in_pixel);
    } else {
      painter->drawLine(thickness, y_in_pixel, thickness - 5, y_in_pixel);
    }
    y += step * document().scale();
  }
  y = document().scroll().y();
  for (int i = 0;; i--) {
    if (y < 0) {
      break;
    }
    int y_in_pixel = round(y);
    if (i % 10 == 0) {
      QString verticalValue = TextToVertical(QString::number(i * step / 10));
      QRect rect{1, y_in_pixel + 5, 10, 100};
      painter->drawText(rect, 0, verticalValue);
      painter->drawLine(thickness, y_in_pixel, 0, y_in_pixel);
    } else if (i % 2 == 0) {
      painter->drawLine(thickness, y_in_pixel, thickness - 10, y_in_pixel);
    } else {
      painter->drawLine(thickness, y_in_pixel, thickness - 5, y_in_pixel);
    }
    y -= step * document().scale();
  }

  painter->drawLine(thickness, thickness, thickness, canvas().height());
}

void Ruler::paint(QPainter *painter) {

  qreal ruler_width = 20;

  qreal step = getScaleStep();

  QPen line_pen;
  line_pen.setWidth(1);
  line_pen.setStyle(Qt::SolidLine);
  QColor ruler_color;
  if (isDarkMode()) {
    line_pen.setColor("#F0F0F0");
    ruler_color.setNamedColor("#454545");
  } else {
    line_pen.setColor("#333");
    ruler_color.setNamedColor("#DDD");
  }
  line_pen.setCosmetic(true);

  drawHorizontalRuler(painter, step, ruler_width, line_pen, ruler_color);
  drawVerticalRuler(painter, step, ruler_width, line_pen, ruler_color);

  painter->fillRect(QRectF{0, 0, ruler_width, ruler_width}, ruler_color);
}
