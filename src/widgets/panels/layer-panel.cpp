#include "layer-panel.h"
#include "ui_layer-panel.h"
#include <canvas/canvas.h>
#include <windows/mainwindow.h>
#include <QAbstractItemView>
#include <boost/range/adaptor/reversed.hpp>
#include <windows/osxwindow.h>

LayerPanel::LayerPanel(QWidget *parent, MainWindow *main_window) :
     QFrame(parent),
     main_window_(main_window),
     ui(new Ui::LayerPanel),
     BaseContainer() {
  ui->setupUi(this);
  initializeContainer();
  updateLayers();
}

void LayerPanel::loadStyles() {
}

void LayerPanel::loadWidgets() {
  layer_params_panel_ = new LayerParamsPanel(this, main_window_);
  ui->scrollAreaWidgetContents->layout()->addWidget(layer_params_panel_);
}

void LayerPanel::registerEvents() {
  connect(ui->layerList->model(), &QAbstractItemModel::rowsMoved, this, &LayerPanel::layerOrderChanged);
  connect(main_window_->canvas(), &Canvas::layerChanged, this, &LayerPanel::updateLayers);
  connect(ui->layerList, &QListWidget::itemClicked, [=](QListWidgetItem *item) {
    // TODO (Add more UI logic here to prevent redrawing all list widget)
    main_window_->canvas()->setActiveLayer(dynamic_cast<LayerListItem *>(ui->layerList->itemWidget(item))->layer_);
  });
}

void LayerPanel::hideEvent(QHideEvent *event) {
  Q_EMIT panelShow(false);
}

void LayerPanel::showEvent(QShowEvent *event) {
  Q_EMIT panelShow(true);
}

void LayerPanel::resizeEvent(QResizeEvent *e) {
}


void LayerPanel::updateLayers() {
  ui->layerList->clear();

  for (auto &layer : boost::adaptors::reverse(main_window_->canvas()->document().layers())) {
    bool active = main_window_->canvas()->document().activeLayer() == layer.get();
    LayerPtr editable_layer = layer;
    auto *list_widget = new LayerListItem(ui->layerList->parentWidget(),
                                          main_window_->canvas(),
                                          editable_layer,
                                          active);
    auto *list_item = new QListWidgetItem(ui->layerList);
    auto size = list_widget->size();
    list_item->setSizeHint(size);
    ui->layerList->setItemWidget(list_item, list_widget);

    if (active) {
      ui->layerList->setCurrentItem(list_item);
    }
  }

  if (ui->layerList->currentItem()) {
    ui->layerList->scrollToItem(ui->layerList->currentItem(), QAbstractItemView::PositionAtCenter);
  }

  layer_params_panel_->updateLayer(main_window_->canvas()->document().activeLayer());
}

void LayerPanel::layerOrderChanged(const QModelIndex &sourceParent, int sourceStart, int sourceEnd,
                                   const QModelIndex &destinationParent, int destinationRow) {
  QList<LayerPtr> new_order;

  for (int i = ui->layerList->count() - 1; i >= 0; i--) {
    new_order << dynamic_cast<LayerListItem *>(ui->layerList->itemWidget(ui->layerList->item(i)))->layer_;
  }

  main_window_->canvas()->setLayerOrder(new_order);
}

LayerPanel::~LayerPanel() {
  delete ui;
}
