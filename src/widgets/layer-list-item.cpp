#include <QDebug>
#include <QStyleOption>
#include <QInputDialog>
#include <QColorDialog>
#include <QLineEdit>
#include <widgets/layer-list-item.h>
#include <canvas/canvas.h>
#include "ui_layer-list-item.h"
#include <command.h>

LayerListItem::LayerListItem(QWidget *parent, Canvas *canvas, LayerPtr &layer, bool active) :
     QWidget(parent),
     canvas_(canvas),
     ui(new Ui::LayerListItem),
     layer_(layer),
     active_(active) {
  ui->setupUi(this);
  ui->labelName->setText(layer->name());
  ui->comboBox->setCurrentIndex((int) layer->type());
  createIcon();
  loadStyles();
  registerEvents();
}

LayerListItem::~LayerListItem() {
  delete ui;
}

void LayerListItem::createIcon() {
  QPixmap pix(100, 100);
  pix.fill(QColor::fromRgba64(0, 0, 0, 0));
  QPainter paint(&pix);
  paint.setRenderHint(QPainter::Antialiasing, true);
  QPen pen(QColor(255, 255, 255, 255), 5);
  paint.setPen(pen);
  paint.setBrush(QBrush(layer_->color()));
  paint.drawRoundedRect(QRectF(30, 30, 40, 40), 10, 10);
  paint.end();
  ui->labelIcon->setPixmap(pix);
}

void LayerListItem::loadStyles() {
  if (active_) {
    ui->layerWidgetFrame->setStyleSheet("#layerWidgetFrame { background-color: #0091ff; }");
    ui->labelName->setStyleSheet("color: white;");
  }
}

void LayerListItem::registerEvents() {
  connect(ui->btnHide, &QAbstractButton::clicked, [=]() {
    layer_->document().execute(
         Commands::Set<Layer, bool, &Layer::isVisible, &Layer::setVisible>(layer_.get(), !layer_->isVisible())
    );
  });
  connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
    layer_->document().execute(
         Commands::Set<Layer, Layer::Type, &Layer::type, &Layer::setType>(layer_.get(), (Layer::Type) index)
    );
  });
}

void LayerListItem::mouseDoubleClickEvent(QMouseEvent *event) {
  if (event->localPos().x() < 60) {
    QColor color = QColorDialog::getColor(layer_->color(), this, "Set layer color");
    if (color != layer_->color()) {
      layer_->document().execute(
           Commands::SetRef<Layer, QColor, &Layer::color, &Layer::setColor>(layer_.get(), color)
      );
    }
  } else {
    bool ok;
    QString text = QInputDialog::getText(this, "Rename Layer",
                                         tr("Layer name:"), QLineEdit::Normal,
                                         layer_->name(), &ok);
    if (ok && !text.isEmpty() && text != layer_->name()) {
      ui->labelName->setText(text);
      layer_->document().execute(
           Commands::SetRef<Layer, QString, &Layer::name, &Layer::setName>(layer_.get(), text)
      );
    }
  }
  // TODO (Fix event flow)
  emit canvas_->layerChanged();
}

void LayerListItem::paintEvent(QPaintEvent *event) {
  QStyleOption opt;
  opt.initFrom(this);
  QPainter p(this);
  style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
  QWidget::paintEvent(event);
}
