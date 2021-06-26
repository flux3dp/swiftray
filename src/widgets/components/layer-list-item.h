#ifndef LAYER_WIDGET_H
#define LAYER_WIDGET_H

#include <QWidget>
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

  Canvas *canvas_;
  Ui::LayerListItem *ui;
  bool active_;
};

#endif // LAYER_WIDGET_H
