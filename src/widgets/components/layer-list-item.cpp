#include <QDebug>
#include <QStyleOption>
#include <QInputDialog>
#include <QColorDialog>
#include <QLineEdit>
#include <widgets/components/layer-list-item.h>
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

  setContextMenu();
  createIcon();
  loadStyles();
  registerEvents();

  ui->btnLock->setVisible(layer_->isLocked());
  lockLayerAction_->setEnabled(!layer_->isLocked());
  unlockLayerAction_->setEnabled(layer_->isLocked());
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
         Commands::Set<Layer, bool, &Layer::isVisible, &Layer::setVisible>(layer_.get(), !layer_->isVisible()),
         Commands::Select(&(layer_->document()), {})
    );
  });
  connect(ui->btnLock, &QAbstractButton::clicked, this, &LayerListItem::onUnlockLayer);
  connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
    layer_->document().execute(
         Commands::Set<Layer, Layer::Type, &Layer::type, &Layer::setType>(layer_.get(), (Layer::Type) index)
    );
  });
}

void LayerListItem::setContextMenu() {
  popMenu_ = new QMenu(this);
  // Add QActions for context menu
  lockLayerAction_ =  popMenu_->addAction(tr("&Lock"));
  unlockLayerAction_ =  popMenu_->addAction(tr("&Unlock"));
  addAction(lockLayerAction_);
  addAction(unlockLayerAction_);
  connect(lockLayerAction_, &QAction::triggered, this, &LayerListItem::onLockLayer);
  connect(unlockLayerAction_, &QAction::triggered, this, &LayerListItem::onUnlockLayer);

  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, &QWidget::customContextMenuRequested,
          this, &LayerListItem::showPopMenu);
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

void LayerListItem::onLockLayer() {
  layer_->document().setSelection(nullptr);
  layer_->setLocked(true);
  ui->btnLock->setVisible(true);
  lockLayerAction_->setEnabled(false);
  unlockLayerAction_->setEnabled(true);
}

void LayerListItem::onUnlockLayer() {
  layer_->setLocked(false);
  ui->btnLock->setVisible(false);
  lockLayerAction_->setEnabled(true);
  unlockLayerAction_->setEnabled(false);
}

void LayerListItem::showPopMenu(const QPoint& ) // SLOT Function
{
  if(popMenu_){
      popMenu_->exec(QCursor::pos());
  }
}
