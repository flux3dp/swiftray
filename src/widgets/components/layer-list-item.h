#pragma once

#include <QWidget>
#include <QMenu>
#include <layer.h>

namespace Ui {
  class LayerListItem;
}

class LayerListItem : public QWidget {
Q_OBJECT

public:
  // explicit LayerListItem(QWidget *parent = nullptr);
  LayerListItem(QWidget *parent, Canvas *canvas, LayerPtr &layer, bool active);

  void paintEvent(QPaintEvent *event) override;

  ~LayerListItem();

  LayerPtr layer_;

  void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
  void createIcon();

  void loadStyles();

  void registerEvents();

  void setContextMenu();

  void showPopMenu(const QPoint& );

  void onLockLayer();

  void onUnlockLayer();

  Canvas *canvas_;
  QMenu *popMenu_;
  QAction *lockLayerAction_;
  QAction *unlockLayerAction_;

  Ui::LayerListItem *ui;
  bool active_;
};