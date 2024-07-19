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
  qreal x_anchor = document().scroll().x();
  qreal x;
  for (int i = 0;; i++) {
    x = x_anchor + i * step * document().scale();
    if (x > canvas().width()) {
      break;
    }

    if (i % 10 == 0) {
      painter->drawText(QPointF{double(x+5),  10}, QString::number(i * step / 10));
      painter->drawLine(QLineF{x, double(thickness), x, 0});
    } else if (i % 2 == 0) {
      painter->drawLine(QLineF{x, double(thickness), x, double(thickness - 10)});
    } else {
      painter->drawLine(QLineF{x, double(thickness), x, double(thickness - 5)});
    }
  }
  for (int i = 0;; i--) {
    x = x_anchor + i * step * document().scale();
    if (x < 0) {
      break;
    }

    if (i % 10 == 0) {
      painter->drawText(QPointF{double(x+5),  10}, QString::number(i * step / 10));
      painter->drawLine(QLineF{x, double(thickness), x, 0});
    } else if (i % 2 == 0) {
      painter->drawLine(QLineF{x, double(thickness), x, double(thickness - 10)});
    } else {
      painter->drawLine(QLineF{x, double(thickness), x, double(thickness - 5)});
    }
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
  qreal y_anchor = document().scroll().y();
  qreal y;
  for (int i = 0;; i++) {
    y = y_anchor + i * step * document().scale();
    if (y > canvas().height()) {
      break;
    }

    if (i % 10 == 0) {
      QString verticalValue = TextToVertical(QString::number(i * step / 10));
      QRectF rect{1, y + 5, 10, 100};
      painter->drawText(rect, 0, verticalValue);
      painter->drawLine(QLineF{double(thickness), y, 0, y});
    } else if (i % 2 == 0) {
      painter->drawLine(QLineF{double(thickness), y, double(thickness - 10), y});
    } else {
      painter->drawLine(QLineF{double(thickness), y, double(thickness - 5), y});
    }
  }
  for (int i = 0;; i--) {
    y = y_anchor + i * step * document().scale();
    if (y < 0) {
      break;
    }

    if (i % 10 == 0) {
      QString verticalValue = TextToVertical(QString::number(i * step / 10));
      QRectF rect{1, y + 5, 10, 100};
      painter->drawText(rect, 0, verticalValue);
      painter->drawLine(QLineF{double(thickness), y, 0, y});
    } else if (i % 2 == 0) {
      painter->drawLine(QLineF{double(thickness), y, double(thickness - 10), y});
    } else {
      painter->drawLine(QLineF{double(thickness), y, double(thickness - 5), y});
    }
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
    ruler_color.fromString("#454545");
  } else {
    line_pen.setColor("#333");
    ruler_color.fromString("#DDD");
  }
  line_pen.setCosmetic(true);

  drawHorizontalRuler(painter, step, ruler_width, line_pen, ruler_color);
  drawVerticalRuler(painter, step, ruler_width, line_pen, ruler_color);

  painter->fillRect(QRectF{0, 0, ruler_width, ruler_width}, ruler_color);
}
