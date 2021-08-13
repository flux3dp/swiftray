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
  while (step * document().scale() >= 20) {
    int result;

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


void Ruler::drawHorizontalRuler(QPainter *painter, qreal step, int thickness) {
  QPen black_pen(QColor("#333"), 1, Qt::SolidLine);
  black_pen.setCosmetic(true);

  painter->fillRect(QRectF{double(thickness), 0, canvas().width() - thickness, double(thickness)}, QColor("#DDD"));

  painter->setPen(black_pen);
  qreal x = document().scroll().x();
  for (int i = 0;; i++) {
    if (x > canvas().width()) {
      break;
    }
    int x_in_pixel = int(x);
    if (i % 10 == 0) {
      painter->drawText(QPointF{double(x_in_pixel+5),  10}, QString::number(i * step / 10));
      painter->drawLine(x_in_pixel, thickness, x_in_pixel, 0);
    } else if (i % 2 == 0) {
      painter->drawLine(x_in_pixel, thickness, x_in_pixel, thickness - 10);
    } else {
      painter->drawLine(x_in_pixel, thickness, x_in_pixel, thickness - 5);
    }
    x += step*document().scale();
  }
  x = document().scroll().x();
  for (int i = 0;; i--) {
    if (x < 0) {
      break;
    }
    int x_in_pixel = int(x);
    if (i % 10 == 0) {
      painter->drawText(QPointF{double(x_in_pixel+5),  10}, QString::number(i * step / 10));
      painter->drawLine(x_in_pixel, thickness, x_in_pixel, 0);
    } else if (i % 2 == 0) {
      painter->drawLine(x_in_pixel, thickness, x_in_pixel, thickness - 10);
    } else {
      painter->drawLine(x_in_pixel, thickness, x_in_pixel, thickness - 5);
    }
    x -= step*document().scale();
  }

  painter->drawLine(thickness, thickness, canvas().width(), thickness);
}
void Ruler::drawVerticalRuler(QPainter *painter, qreal step, int thickness) {

  auto TextToVertical = [](const QString &str) {
    QString result;
    for (auto it = str.constBegin(); it != str.constEnd(); it++) {
      result += *it;
      result += "\n";
    }
    return result;
  };

  QPen black_pen(QColor("#333"), 1, Qt::SolidLine);
  black_pen.setCosmetic(true);

  painter->fillRect(QRectF{0, double(thickness), double(thickness), canvas().height() - thickness}, QColor("#DDD"));

  painter->setPen(black_pen);
  qreal y = document().scroll().y();
  for (int i = 0;; i++) {
    if (y > canvas().height()) {
      break;
    }
    int y_in_pixel = int(y);
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
    y += step*document().scale();
  }
  y = document().scroll().y();
  for (int i = 0;; i--) {
    if (y < 0) {
      break;
    }
    int y_in_pixel = int(y);
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
    y -= step*document().scale();
  }

  painter->drawLine(thickness, thickness, thickness, canvas().height());
}

void Ruler::paint(QPainter *painter) {

  qreal ruler_width = 20;

  int step = getScaleStep();

  drawHorizontalRuler(painter, step, ruler_width);
  drawVerticalRuler(painter, step, ruler_width);

  painter->fillRect(QRectF{0, 0, ruler_width, ruler_width}, QColor("#DDD"));
}
