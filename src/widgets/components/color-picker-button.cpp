#include "color-picker-button.h"
#include "ui_color-picker-button.h"

#include <QColorDialog>
#include <QPainter>

ColorPickerButton::ColorPickerButton(QColor color, QWidget *parent) :
        QToolButton(parent), ui(new Ui::ColorPickerButton) {
  ui->setupUi(this);
  // NOTE: styles for this button (e.g. border and size) are actually set in layer-list-item.ui
  //setIconSize(QSize(16, 16));
  //setFixedWidth(16);
  setStyleSheet(QString::fromUtf8(""));
  setBackgroundRole(QPalette::Button);
  setAutoFillBackground(false);
  setToolButtonStyle(Qt::ToolButtonIconOnly);

  color_ = color;
  updateIcon(color_);

  registerEvents();
}

ColorPickerButton::ColorPickerButton(QWidget *parent) :
    ColorPickerButton(QColor{1, 1, 1}, parent) {
}

ColorPickerButton::~ColorPickerButton() {
  delete ui;
}

void ColorPickerButton::setTitle(QString title) {
  this->title_ = title;
}

void ColorPickerButton::setColor(QColor new_color) {
  updateIcon(new_color);
}

void ColorPickerButton::registerEvents() {
  connect(this, &QAbstractButton::clicked, [=](){
    QColor color = QColorDialog::getColor(this->color_, this, this->title_);
    if (!color.isValid()) { // Canceled
      return;
    }
    if (color != this->color_) {
      this->updateIcon(color);
      Q_EMIT colorChanged(this->color_);
    }
  });
}

/**
 * @brief Draw the new color icon with new color 
 * @param color new selected color
 */
void ColorPickerButton::updateIcon(QColor color) {
  this->color_ = color;

  QPixmap pix(40, 40);
  pix.fill(QColor::fromRgba64(0, 0, 0, 0));
  QPainter paint(&pix);
  paint.setRenderHint(QPainter::Antialiasing, true);
  QPen pen(QColor(255, 255, 255, 255), 4);
  paint.setPen(pen);
  paint.setBrush(QBrush(this->color_));
  paint.drawRoundedRect(QRectF(2, 2, 36, 36), 12, 12);
  paint.end();
  QIcon ButtonIcon(pix);
  this->setIcon(ButtonIcon);
}
