#include <QDoubleSpinBox>
#include <QDebug>

#pragma once

class QDoubleSpinBox2 : public QDoubleSpinBox {
public:
  QDoubleSpinBox2(QWidget *parent) : QDoubleSpinBox(parent) {}

  QString textFromValue(double value) const override {
    if (value - qRound(value) == 0) {
      return QString::number(value) + ".0";
    } else {
      return QString::number(value);
    }
  }
};
