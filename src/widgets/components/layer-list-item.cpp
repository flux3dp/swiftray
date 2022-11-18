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
  initColorIcon(layer_->color());
  loadStyles();
  registerEvents();

  // Make lock button occupy space even when hidden
  QSizePolicy sp_retain = ui->btnLock->sizePolicy();
  sp_retain.setRetainSizeWhenHidden(true);
  ui->btnLock->setSizePolicy(sp_retain);

  ui->btnLock->setVisible(layer_->isLocked());
  ui->btnHide->setChecked(!layer_->isVisible());
  lockLayerAction_->setEnabled(!layer_->isLocked());
  unlockLayerAction_->setEnabled(layer_->isLocked());
}

LayerListItem::~LayerListItem() {
  delete ui;
}

void LayerListItem::initColorIcon(QColor color) {
  ui->btnColorPicker->setColor(color);
  ui->btnColorPicker->setTitle(tr("Set layer color"));
}

void LayerListItem::loadStyles() {
  if (active_) {
    ui->layerWidgetFrame->setStyleSheet("#layerWidgetFrame { background-color: #0091ff; }");
    ui->labelName->setStyleSheet("color: white;");
  }
}

void LayerListItem::registerEvents() {
  connect(ui->btnColorPicker, &ColorPickerButton::colorChanged, [=](QColor new_color) {
    layer_->document().execute(
            Commands::SetRef<Layer, QColor, &Layer::color, &Layer::setColor>(layer_.get(), new_color)
    );
    Q_EMIT canvas_->layerChanged();
  });
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
  renameLayerAction_ = popMenu_->addAction(tr("Rename"));
  lockLayerAction_ =  popMenu_->addAction(tr("&Lock"));
  unlockLayerAction_ =  popMenu_->addAction(tr("&Unlock"));
  duplicateLayerAction_ = popMenu_->addAction(tr("Duplicate"));
  deleteLayerAction_ = popMenu_->addAction(tr("Delete"));

  addAction(renameLayerAction_);
  addAction(lockLayerAction_);
  addAction(unlockLayerAction_);
  addAction(duplicateLayerAction_);
  addAction(deleteLayerAction_);

  connect(renameLayerAction_, &QAction::triggered, this, &LayerListItem::onRenameLayer);
  connect(lockLayerAction_, &QAction::triggered, this, &LayerListItem::onLockLayer);
  connect(unlockLayerAction_, &QAction::triggered, this, &LayerListItem::onUnlockLayer);
  connect(duplicateLayerAction_, &QAction::triggered, this, &LayerListItem::onDuplicateLayer);
  connect(deleteLayerAction_, &QAction::triggered, this, &LayerListItem::onDeleteLayer);

  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, &QWidget::customContextMenuRequested,
          this, &LayerListItem::showPopMenu);
}

void LayerListItem::mouseDoubleClickEvent(QMouseEvent *event) {
  onRenameLayer();
  // TODO (Fix event flow?)
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
  renameLayerAction_->setEnabled(true);
  duplicateLayerAction_->setEnabled(true);
  deleteLayerAction_->setEnabled(true);
}

void LayerListItem::onUnlockLayer() {
  layer_->setLocked(false);
  ui->btnLock->setVisible(false);
  lockLayerAction_->setEnabled(true);
  unlockLayerAction_->setEnabled(false);
  renameLayerAction_->setEnabled(true);
  duplicateLayerAction_->setEnabled(true);
  deleteLayerAction_->setEnabled(true);
}

void LayerListItem::onRenameLayer() {
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
  // TODO (Fix event flow?)
  Q_EMIT canvas_->layerChanged();
}

void LayerListItem::onDuplicateLayer() {
  canvas_->duplicateLayer(layer_);
}

void LayerListItem::onDeleteLayer() {
  if (layer_->document().layers().length() == 1) { // should add one default layer when no layer in the list
    canvas_->addEmptyLayer();
    layer_->document().execute(
      // Commands::RemoveSelections(&(layer_->document())),
      Commands::RemoveLayer(layer_)
    );
  } else {
    bool change_layer = false;
    if(layer_->name() == layer_->document().activeLayer()->name()) {
      change_layer = true;
    }
    layer_->document().execute(
      // Commands::RemoveSelections(&(layer_->document())),
      Commands::RemoveLayer(layer_)
    );
    if(change_layer) {
      LayerPtr last_layer = canvas_->document().layers().last();
      canvas_->setActiveLayer(last_layer);
      canvas_->document().setSelection(nullptr);
    }
  }
  Q_EMIT canvas_->layerChanged();
}

void LayerListItem::showPopMenu(const QPoint& ) // SLOT Function
{
  if(popMenu_){
      popMenu_->exec(QCursor::pos());
  }
}
